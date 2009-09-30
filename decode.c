#include "decode.h"
#include "answer.h"
#include "comment.h"
#include "database.h"
#include "image.h"
#include "question.h"
#include "section.h"
#include "topic.h"
#include "traffreg.h"
#include "delphi_helper.h"
#include "yaml_helper.h"

#include <glib/gstdio.h>
#include <string.h>
#include <unistd.h>
#include <yaml.h>

typedef struct topic_question_s
{
	gint8 topic_number;
	gint32 question_offset;
} __attribute__((__packed__)) topic_question_t;

extern gchar *program_dir;

guint16 magic = 0;

gboolean init_magic(const gchar *root_path);
gboolean decode_images(const gchar *root_path);
gboolean decode_comments(const gchar *root_path);
gboolean decode_traffregs(const gchar *root_path);
gboolean decode_questions(const gchar *root_path);

gboolean decode(const gchar *root_path)
{
	if (!init_magic(root_path))
	{
		return FALSE;
	}

	if (!decode_images(root_path))
	{
		return FALSE;
	}

	if (!decode_comments(root_path))
	{
		return FALSE;
	}

	if (!decode_traffregs(root_path))
	{
		return FALSE;
	}

	if (!decode_questions(root_path))
	{
		return FALSE;
	}

	return TRUE;
}

gchar *find_file_ci(const gchar *path, const gchar *fname)
{
	GError *err;
	GDir *dir = g_dir_open(path, 0, &err);
	if (!dir)
	{
		g_error("%s\n", err->message);
	}
	gchar *file_path = NULL;
	const gchar *name;
	while ((name = g_dir_read_name(dir)))
	{
		if (!strcasecmp(name, fname))
		{
			file_path = g_build_filename(path, name, NULL);
			break;
		}
	}
	g_dir_close(dir);
	return file_path;
}

gchar *make_path(const gchar *root_path, ...)
{
	gchar *result = g_strdup(root_path);
	va_list list;
	va_start(list, root_path);
	const gchar *path_part;
	while ((path_part = va_arg(list, const gchar *)))
	{
		gchar *path = find_file_ci(result, path_part);
		g_free(result);
		result = path;
		if (!result)
		{
			break;
		}
	}
	va_end(list);
	g_print("make_path: %s\n", result);
	return result;
}

gboolean init_magic(const gchar *root_path)
{
	gchar *pdd32_path = make_path(root_path, "pdd32.exe", NULL);
	FILE *f = g_fopen(pdd32_path, "rb");
	g_free(pdd32_path);
	if (!f)
	{
		return FALSE;
	}

	guchar *buffer = g_malloc(32 * 1024);
	fseek(f, 32 * 1024, SEEK_SET);
	if (fread(buffer, 32 * 1024, 1, f) != 1)
	{
		g_error("pdd32.exe invalid");
	}

	// TODO: get pdd32.exe modification date since "2008" seems to be a release year
	//       another way could be reading date at offset 0x0270 in pdd32.exe (for 10 & 11, it's "PDD-2008")
	magic = 0x2008;
	set_randseed((buffer[0] | (buffer[1] << 8)) & 0x0ffff);

	while (!feof(f))
	{
		int length = fread(buffer, 1, 32 * 1024, f);
		if (length <= 0)
		{
			break;
		}
		int i, j;
		for (i = 0; i < 256; i++)
		{
			guchar ch = buffer[delphi_random(length)];
			for (j = 0; j < 8; j++)
			{
				guint16 old_magic = magic;
				magic >>= 1;
				if ((ch ^ old_magic) & 1)
				{
					// TODO: magic number?
					magic ^= 0x0a001;
				}
				ch >>= 1;
			}
		}
	}

	g_print("magic: 0x%04x\n", magic);

	g_free(buffer);
	fclose(f);
	return TRUE;
}

gboolean decode_image(const gchar *path)
{
	gchar *basename = g_path_get_basename(path);
	init_randseed_for_image(basename, magic);

	GError *err;
	gchar *data;
	gsize data_length, i;

	if (!g_file_get_contents(path, &data, &data_length, &err))
	{
		g_error("%s\n", err->message);
	}

	if (strncmp(data, "BPFT", 4))
	{
		g_error("unknown image format\n");
	}

	for (i = 4; i < data_length; i++)
	{
		data[i] ^= delphi_random(255);
	}

	pdd_image_t *image = image_new(g_strdelimit(basename, ".", '\0'), data + 4, data_length - 4);
	gboolean result = image_save(image);
	image_free(image);
	g_free(basename);
	g_free(data);

	if (!result)
	{
		g_error("unable to save image");
	}
	
	return result;
}

gboolean decode_images(const gchar *root_path)
{
	yaml_parser_t parser;
	FILE *f = yaml_open_file();
	if (!f)
	{
		g_error("unable to open YAML file\n");
	}
	if (!yaml_init_and_find_node(&parser, f, "image_dirs"))
	{
		return FALSE;
	}
	if (yaml_get_event(&parser, NULL) != YAML_SEQUENCE_START_EVENT)
	{
		g_error("malformed YAML\n");
	}
	gboolean result = TRUE;
	database_tx_begin();
	while (TRUE)
	{
		gchar *dir_name;
		yaml_event_type_t event = yaml_get_event(&parser, &dir_name);
		if (event == YAML_SEQUENCE_END_EVENT)
		{
			g_free(dir_name);
			break;
		}
		if (event != YAML_SCALAR_EVENT)
		{
			g_error("malformed YAML\n");
		}

		gchar *images_path = make_path(root_path, dir_name, NULL);
		GError *err = NULL;
		GDir *dir = g_dir_open(images_path, 0, &err);
		if (!dir)
		{
			g_error("%s\n", err->message);
		}

		const gchar *name;
		while ((name = g_dir_read_name(dir)))
		{
			gchar *image_path = g_build_filename(images_path, name, NULL);
			result = decode_image(image_path);
			g_free(image_path);
			if (!result)
			{
				break;
			}
		}

		g_dir_close(dir);
		g_free(images_path);

		if (!result)
		{
			break;
		}
	}
	database_tx_commit();
	yaml_parser_delete(&parser);
	fclose(f);
	return result;
}

gint32 *decode_table(const gchar *path, gsize *table_size)
{
	GError *err = NULL;
	gint32 *table;
	if (!g_file_get_contents(path, (gchar **)&table, table_size, &err))
	{
		g_error("%s\n", err->message);
	}

	if (*table_size % sizeof(gint32))
	{
		g_error("invalid file size: %s (should be multiple of %u)\n", path, (guint)sizeof(gint32));
	}

	*table_size /= sizeof(gint32);

	gsize i;
	for (i = 0; i < *table_size; i++)
	{
		if (table[i] != -1)
		{
			table[i] ^= magic;
			// delphi has file offsets starting from 1, we need 0
			table[i]--;
		}
	}

	return table;
}

topic_question_t *decode_topic_questions_table(const gchar *path, gsize *table_size)
{
	GError *err = NULL;
	topic_question_t *table;
	if (!g_file_get_contents(path, (gchar **)&table, table_size, &err))
	{
		g_error("%s\n", err->message);
	}

	if (*table_size % sizeof(topic_question_t))
	{
		g_error("invalid file size: %s (should be multiple of %u)\n", path, (guint)sizeof(topic_question_t));
	}

	*table_size /= sizeof(topic_question_t);

	gsize i;
	for (i = 0; i < *table_size; i++)
	{
		table[i].question_offset ^= magic;
		// delphi has file offsets starting from 1, we need 0
		// have to subtract another 1 from it (tell me why it points to 'R', not '[')
		table[i].question_offset -= 2;
	}

	return table;
}

gchar *decode_string(const gchar *path, gsize *str_size, gint8 topic_number)
{
	GError *err = NULL;
	gchar *str;
	if (!g_file_get_contents(path, &str, str_size, &err))
	{
		g_error("%s\n", err->message);
	}

	gint32 i;
	for (i = 0; i < *str_size; i++)
	{
		// TODO: magic numbers?
		str[i] ^= (magic & 0x0ff) ^ topic_number ^ (i & 1 ? 0x30 : 0x16) ^ ((i + 1) % 255);
	}

	return str;
}

typedef gpointer (*object_new_t)(gint32, const gchar *);
typedef gboolean (*object_save_t)(gpointer *);
typedef void (*object_free_t)(gpointer *);
typedef void (*object_set_images_t)(gpointer *, pdd_images_t *images);

gboolean decode_simple_data(const gchar *dat_path, const gchar *dbt_path,
	object_new_t object_new, object_save_t object_save, object_free_t object_free, object_set_images_t object_set_images)
{
	gsize table_size;
	gint32 *table = decode_table(dat_path, &table_size);

	gsize str_size;
	gchar *str = decode_string(dbt_path, &str_size, 0);

	GError *err = NULL;
	GRegex *simple_data_regex = g_regex_new("^#(\\d+)\\s*((?:&[a-zA-Z0-9_-]+\\s*)*)(.+)$", G_REGEX_OPTIMIZE | G_REGEX_DOTALL, 0, &err);
	if (err)
	{
		g_error("%s\n", err->message);
	}
	/*GRegex *word_break_regex = g_regex_new("(?<!\\s)-\\s+", G_REGEX_OPTIMIZE, G_REGEX_MATCH_NEWLINE_ANY, &err);
	if (err)
	{
		g_error("%s\n", err->message);
	}
	GRegex *spaces_regex = g_regex_new("\\s{2,}", G_REGEX_OPTIMIZE, G_REGEX_MATCH_NEWLINE_ANY, &err);
	if (err)
	{
		g_error("%s\n", err->message);
	}*/

	gsize i, j;
	gboolean result = TRUE;
	database_tx_begin();
	for (i = 0; i < table_size; i++)
	{
		gpointer *object = NULL;
		pdd_images_t *object_images = g_ptr_array_new();
		if (table[i] == -1)
		{
			object = object_new(table[i], NULL);
		}
		else
		{
			gint32 next_offset = str_size;
			for (j = 0; j < table_size; j++)
			{
				if (table[j] > table[i] && table[j] < next_offset)
				{
					next_offset = table[j];
				}
			}

			gchar *data = g_convert(str + table[i], next_offset - table[i], "utf-8", "cp1251", NULL, NULL, &err);
			if (!data)
			{
				g_error("%s\n", err->message);
			}
			GMatchInfo *match_info;
			if (!g_regex_match(simple_data_regex, data, 0, &match_info))
			{
				g_error("unable to match simple data\n");
			}
			gchar *number = g_strchomp(g_match_info_fetch(match_info, 1));
			gchar *images = g_strchomp(g_match_info_fetch(match_info, 2));
			gchar *text = g_strchomp(g_match_info_fetch(match_info, 3));
			if (*images)
			{
				gchar **image_names = g_strsplit(images, "\n", 0);
				gchar **in = image_names;
				while (*in)
				{
					pdd_image_t *image = image_find_by_name(g_strchomp(*in + 1));
					if (!image)
					{
						g_error("unable to find image with name %s\n", *in + 1);
					}
					g_ptr_array_add(object_images, image);
					in++;
				}
				g_strfreev(image_names);
			}
			object = object_new(atoi(number), g_strchomp(text));
			g_free(text);
			g_free(images);
			g_free(number);
			g_match_info_free(match_info);
			g_free(data);
		}
		result = object_save(object);
		if (object_set_images)
		{
			object_set_images(object, object_images);
		}
		image_free_all(object_images);
		if (!result)
		{
			g_error("unable to save object\n");
		}
		object_free(object);
		g_print(".");
	}
	database_tx_commit();

	g_print("\n");

	//g_regex_unref(spaces_regex);
	//g_regex_unref(word_break_regex);
	g_regex_unref(simple_data_regex);
	g_free(str);
	g_free(table);

	return result;
}

gboolean decode_comments(const gchar *root_path)
{
	gchar *comments_dat_path = make_path(root_path, "tickets", "comments", "comments.dat", NULL);
	gchar *comments_dbt_path = make_path(root_path, "tickets", "comments", "comments.dbt", NULL);

	gboolean result = decode_simple_data(comments_dat_path, comments_dbt_path,
		(object_new_t)comment_new, (object_save_t)comment_save, (object_free_t)comment_free, NULL);

	if (!result)
	{
		g_error("unable to decode comments\n");
	}

	g_free(comments_dat_path);
	g_free(comments_dbt_path);

	return result;
}

gboolean decode_traffregs(const gchar *root_path)
{
	gchar *traffreg_dat_path = make_path(root_path, "tickets", "traffreg", "traffreg.dat", NULL);
	gchar *traffreg_dbt_path = make_path(root_path, "tickets", "traffreg", "traffreg.dbt", NULL);

	gboolean result = decode_simple_data(traffreg_dat_path, traffreg_dbt_path,
		(object_new_t)traffreg_new, (object_save_t)traffreg_save, (object_free_t)traffreg_free,
		(object_set_images_t)traffreg_set_images);

	if (!result)
	{
		g_error("unable to decode traffregs");
	}

	g_free(traffreg_dat_path);
	g_free(traffreg_dbt_path);

	return result;
}

gboolean decode_questions_data(const gchar *dbt_path, gint8 topic_number, topic_question_t *sections_data, gsize sections_data_size)
{
	topic_question_t *table = sections_data;
	while (table->topic_number != topic_number)
	{
		table++;
		sections_data_size--;
	}
	gsize table_size = 0;
	while (table_size < sections_data_size && table[table_size].topic_number == topic_number)
	{
		table_size++;
	}

	gsize str_size;
	gchar *str = decode_string(dbt_path, &str_size, topic_number);

	GError *err = NULL;
	GRegex *question_data_regex = g_regex_new("\\s*\\[|\\]\\s*", G_REGEX_OPTIMIZE, G_REGEX_MATCH_NEWLINE_ANY, &err);
	if (err)
	{
		g_error("%s\n", err->message);
	}
	GRegex *answers_regex = g_regex_new("^($^$)?\\d\\.?\\s+", G_REGEX_OPTIMIZE | G_REGEX_MULTILINE, G_REGEX_MATCH_NEWLINE_ANY, &err);
	if (err)
	{
		g_error("%s\n", err->message);
	}
	GRegex *word_break_regex = g_regex_new("(?<!\\s)-\\s+", G_REGEX_OPTIMIZE, G_REGEX_MATCH_NEWLINE_ANY, &err);
	if (err)
	{
		g_error("%s\n", err->message);
	}
	GRegex *spaces_regex = g_regex_new("\\s{2,}", G_REGEX_OPTIMIZE, G_REGEX_MATCH_NEWLINE_ANY, &err);
	if (err)
	{
		g_error("%s\n", err->message);
	}

	gsize i, j;
	gboolean result = TRUE;
	database_tx_begin();
	for (i = 0; i < table_size; i++)
	{
		gint32 next_offset = str_size;
		for (j = 0; j < table_size; j++)
		{
			if (table[j].question_offset > table[i].question_offset && table[j].question_offset < next_offset)
			{
				next_offset = table[j].question_offset;
			}
		}

		gchar *text = g_convert(str + table[i].question_offset, next_offset - table[i].question_offset, "utf-8", "cp1251", NULL, NULL, &err);
		if (!text)
		{
			g_error("%s\n", err->message);
		}
		gchar **parts = g_regex_split(question_data_regex, text, 0);
		g_free(text);
		gchar **p = parts + 1;
		pdd_question_t *question = question_new(topic_number, NULL, 0, NULL, 0);
		pdd_traffregs_t *question_traffregs = g_ptr_array_new();
		pdd_sections_t *question_sections = g_ptr_array_new();
		pdd_answers_t *question_answers = g_ptr_array_new();
		gint answer_number = 0;
		while (*p)
		{
			switch ((*p)[0])
			{
			case 'R':
				p++;
				{
					gchar **section_names = g_strsplit(*p, " ", 0);
					gchar **sn = section_names;
					while (*sn)
					{
						pdd_section_t *section = section_find_by_name(*sn);
						if (!section)
						{
							g_error("unable to find section with name %s\n", *sn);
						}
						g_ptr_array_add(question_sections, section);
						sn++;
					}
					g_strfreev(section_names);
				}
				break;
			case 'G':
				p++;
				{
					pdd_image_t *image = image_find_by_name(*p);
					if (!image)
					{
						g_error("unable to find image with name %s\n", *p);
					}
					question->image_id = image->id;
					image_free(image);
				}
				break;
			case 'Q':
				p++;
				{
					gchar *question_text = g_regex_replace_literal(word_break_regex, *p, -1, 0, "", 0, &err);
					if (err)
					{
						g_error("%s\n", err->message);
					}
					question->text = g_regex_replace_literal(spaces_regex, question_text, -1, 0, " ", 0, &err);
					g_free(question_text);
					if (err)
					{
						g_error("%s\n", err->message);
					}
					g_strchomp(question->text);
				}
				break;
			case 'W':
			case 'V':
				p++;
				{
					gchar **answers = g_regex_split(answers_regex, *p, 0);
					gchar **a = answers + 1;
					while (*a)
					{
						gchar *answer_text = g_regex_replace_literal(word_break_regex, *a, -1, 0, "", 0, &err);
						if (err)
						{
							g_error("%s\n", err->message);
						}
						pdd_answer_t *answer = answer_new(0, g_regex_replace_literal(spaces_regex, answer_text, -1, 0, " ", 0, &err), FALSE);
						g_free(answer_text);
						if (err)
						{
							g_error("%s\n", err->message);
						}
						g_strchomp(answer->text);
						g_ptr_array_add(question_answers, answer);
						a++;
					}
					g_strfreev(answers);
				}
				break;
			case 'A':
				p++;
				{
					answer_number = atoi(*p) - 1;
				}
				break;
			case 'T':
				p++;
				{
					gchar *advice_text = g_regex_replace_literal(word_break_regex, *p, -1, 0, "", 0, &err);
					if (err)
					{
						g_error("%s\n", err->message);
					}
					question->advice = g_regex_replace_literal(spaces_regex, advice_text, -1, 0, " ", 0, &err);
					g_free(advice_text);
					if (err)
					{
						g_error("%s\n", err->message);
					}
					g_strchomp(question->advice);
				}
				break;
			case 'L':
				p++;
				{
					gchar **traffreg_numbers = g_strsplit(*p, " ", 0);
					gchar **trn = traffreg_numbers;
					while (*trn)
					{
						pdd_traffreg_t *traffreg = traffreg_find_by_number(atoi(*trn));
						if (!traffreg)
						{
							g_error("unable to find traffreg with number %s\n", *trn);
						}
						g_ptr_array_add(question_traffregs, traffreg);
						trn++;
					}
					g_strfreev(traffreg_numbers);
				}
				break;
			case 'C':
				p++;
				{
					pdd_comment_t *comment = comment_find_by_number(atoi(*p));
					if (!comment)
					{
						g_error("unable to find comment with number %s\n", *p);
					}
					question->comment_id = comment->id;
					comment_free(comment);
				}
				break;
			default:
				g_error("unknown question data section: %s\n", *p);
			}
			p++;
		}
		g_strfreev(parts);
		/*g_print("---\n  image_id: %ld\n  text: \"%s\"\n  advice: \"%s\"\n  comment_id: %ld\n  answer_number: %d\n", question->image_id, question->text, question->advice, question->comment_id, answer_number);
		g_print("  answers:");
		if (question_answers->len)
		{
			gsize k;
			for (k = 0; k < question_answers->len; k++)
			{
				g_print("\n    \"%s\"", ((pdd_answer_t *)g_ptr_array_index(question_answers, k))->text);
				if (k == answer_number)
				{
					g_print(" (correct)");
				}
			}
			g_print("\n");
		}
		else
		{
			g_print(" (none)\n");
		}
		g_print("  sections:");
		if (question_sections->len)
		{
			gsize k;
			g_print(" %ld", ((pdd_section_t *)g_ptr_array_index(question_sections, 0))->id);
			for (k = 1; k < question_sections->len; k++)
			{
				g_print(", %ld", ((pdd_section_t *)g_ptr_array_index(question_sections, k))->id);
			}
			g_print("\n");
		}
		else
		{
			g_print(" (none)\n");
		}
		g_print("  traffregs:");
		if (question_traffregs->len)
		{
			gsize k;
			g_print(" %ld", ((pdd_traffreg_t *)g_ptr_array_index(question_traffregs, 0))->id);
			for (k = 1; k < question_traffregs->len; k++)
			{
				g_print(", %ld", ((pdd_traffreg_t *)g_ptr_array_index(question_traffregs, k))->id);
			}
			g_print("\n");
		}
		else
		{
			g_print(" (none)\n");
		}*/
		result = question_save(question);
		if (!result)
		{
			g_error("unable to save question\n");
		}
		gsize k;
		for (k = 0; k < question_answers->len; k++)
		{
			pdd_answer_t *answer = (pdd_answer_t *)g_ptr_array_index(question_answers, k);
			answer->question_id = question->id;
			answer->is_correct = k == answer_number;
			result = answer_save(answer);
			if (!result)
			{
				g_error("unable to save answer\n");
			}
		}
		answer_free_all(question_answers);
		result = question_set_sections(question, question_sections);
		if (!result)
		{
			g_error("unable to set question sections\n");
		}
		section_free_all(question_sections);
		result = question_set_traffregs(question, question_traffregs);
		if (!result)
		{
			g_error("unable to set question traffregs\n");
		}
		traffreg_free_all(question_traffregs);
		question_free(question);
		g_print(".");
	}
	database_tx_commit();

	g_print("\n");

	g_regex_unref(spaces_regex);
	g_regex_unref(word_break_regex);
	g_regex_unref(question_data_regex);
	g_free(str);

	return result;
}

int compare_topic_questions(const void *first, const void *second)
{
	const topic_question_t *first_tq = (const topic_question_t *)first;
	const topic_question_t *second_tq = (const topic_question_t *)second;

	if (first_tq->topic_number != second_tq->topic_number)
	{
		return first_tq->topic_number - second_tq->topic_number;
	}
	if (first_tq->question_offset != second_tq->question_offset)
	{
		return first_tq->question_offset - second_tq->question_offset;
	}
	return 0;
}

gboolean decode_questions(const gchar *root_path)
{
	pdd_sections_t *sections = section_find_all();
	gsize i;
	topic_question_t *sections_data = NULL;
	gsize sections_data_size = 0;
	for (i = 0; i < sections->len; i++)
	{
		pdd_section_t *section = ((pdd_section_t **)sections->pdata)[i];
		gchar *section_dat_name = g_strdup_printf("%s.dat", section->name);
		gchar *section_dat_path = make_path(root_path, "tickets", "parts", section_dat_name, NULL);
		gsize size;
		topic_question_t *data = decode_topic_questions_table(section_dat_path, &size);
		g_free(section_dat_path);
		g_free(section_dat_name);
		if (!data)
		{
			g_error("unable to decode section data\n");
		}
		sections_data = g_realloc(sections_data, (sections_data_size + size) * sizeof(topic_question_t));
		g_memmove(&sections_data[sections_data_size], data, size * sizeof(topic_question_t));
		sections_data_size += size;
		g_free(data);
	}
	section_free_all(sections);

	qsort(sections_data, sections_data_size, sizeof(topic_question_t), compare_topic_questions);

	pdd_topics_t *topics = topic_find_all();
	for (i = 0; i < topics->len; i++)
	{
		pdd_topic_t *topic = ((pdd_topic_t **)topics->pdata)[i];

		gchar *part_dbt_name = g_strdup_printf("part_%d.dbt", topic->number);
		gchar *part_dbt_path = make_path(root_path, "tickets", part_dbt_name, NULL);
	
		gboolean result = decode_questions_data(part_dbt_path, topic->number, sections_data, sections_data_size);
	
		g_free(part_dbt_path);
		g_free(part_dbt_name);
	
		if (!result)
		{
			g_error("unable to decode questions\n");
		}
	}
	topic_free_all(topics);
	return TRUE;
}
