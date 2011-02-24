#include "decode_questions.h"

#include "decode_context.h"

#include "answer.h"
#include "comment.h"
#include "config.h"
#include "image.h"
#include "private/pddby.h"
#include "private/util/aux.h"
#include "private/util/database.h"
#include "private/util/regex.h"
#include "question.h"
#include "section.h"
#include "traffreg.h"

#include <stdlib.h>

pddby_topic_question_t* pddby_decode_topic_questions_table(pddby_decode_context_t* context, char const* path, size_t* table_size)
{
    //GError *err = NULL;
    char* t;
    pddby_topic_question_t* table;
    if (!pddby_aux_file_get_contents(context->pddby, path, &t, table_size))
    {
        //pddby_report_error("");
        //pddby_report_error("%s\n", err->message);
    }

    table = (pddby_topic_question_t*)t;

    if (*table_size % sizeof(pddby_topic_question_t))
    {
        //pddby_report_error("invalid file size: %s (should be multiple of %ld)\n", path, sizeof(pddby_topic_question_t));
    }

    *table_size /= sizeof(pddby_topic_question_t);

    for (size_t i = 0; i < *table_size; i++)
    {
        table[i].question_offset = PDDBY_INT32_FROM_LE(table[i].question_offset) ^ context->data_magic;
        // delphi has file offsets starting from 1, we need 0
        // have to subtract another 1 from it (tell me why it points to 'R', not '[')
        table[i].question_offset -= 2;
    }

    return table;
}

int pddby_decode_questions_data(pddby_decode_context_t* context, char const* dbt_path, int8_t topic_number,
    pddby_topic_question_t* sections_data, size_t sections_data_size)
{
    pddby_topic_question_t* table = sections_data;
    while (table->topic_number != topic_number)
    {
        table++;
        sections_data_size--;
    }
    size_t table_size = 0;
    while (table_size < sections_data_size && table[table_size].topic_number == topic_number)
    {
        table_size++;
    }

    size_t str_size;
    char* str = context->pddby->decode_context->decode_string(context, dbt_path, &str_size, topic_number);

    pddby_regex_t* question_data_regex = pddby_regex_new(context->pddby, "\\s*\\[|\\]\\s*", PDDBY_REGEX_NEWLINE_ANY);
    pddby_regex_t* answers_regex = pddby_regex_new(context->pddby, "^($^$)?\\d\\.?\\s+", PDDBY_REGEX_MULTILINE |
        PDDBY_REGEX_NEWLINE_ANY);
    pddby_regex_t* word_break_regex = pddby_regex_new(context->pddby, "(?<!\\s)-\\s+", PDDBY_REGEX_NEWLINE_ANY);
    pddby_regex_t* spaces_regex = pddby_regex_new(context->pddby, "\\s{2,}", PDDBY_REGEX_NEWLINE_ANY);

    int result = 1;
    pddby_db_tx_begin(context->pddby);
    for (size_t i = 0; i < table_size; i++)
    {
        int32_t next_offset = str_size;
        for (size_t j = 0; j < table_size; j++)
        {
            if (table[j].question_offset > table[i].question_offset && table[j].question_offset < next_offset)
            {
                next_offset = table[j].question_offset;
            }
        }

        char* text = pddby_string_convert(context->iconv, str + table[i].question_offset, next_offset -
            table[i].question_offset);
        if (!text)
        {
            //pddby_report_error("");
            //pddby_report_error("%s\n", err->message);
        }
        char** parts = pddby_regex_split(question_data_regex, text);
        free(text);
        char** p = parts + 1;
        pddby_question_t* question = pddby_question_new(context->pddby, topic_number, NULL, 0, NULL, 0);
        pddby_traffregs_t* question_traffregs = pddby_traffregs_new(context->pddby);
        pddby_sections_t* question_sections = pddby_sections_new(context->pddby);
        pddby_answers_t* question_answers = pddby_answers_new(context->pddby);
        size_t answer_number = 0;
        while (*p)
        {
            switch ((*p)[0])
            {
            case 'R':
                p++;
                {
                    char** section_names = pddby_string_split(*p, " ");
                    char** sn = section_names;
                    while (*sn)
                    {
                        pddby_section_t *section = pddby_section_find_by_name(context->pddby, *sn);
                        if (!section)
                        {
                            //pddby_report_error("unable to find section with name %s\n", *sn);
                        }
                        pddby_array_add(question_sections, section);
                        sn++;
                    }
                    pddby_stringv_free(section_names);
                }
                break;
            case 'G':
                p++;
                {
                    pddby_image_t *image = pddby_image_find_by_name(context->pddby, *p);
                    if (!image)
                    {
                        //pddby_report_error("unable to find image with name %s\n", *p);
                    }
                    question->image_id = image->id;
                    pddby_image_free(image);
                }
                break;
            case 'Q':
                p++;
                {
                    char* question_text = pddby_regex_replace_literal(word_break_regex, *p, "");
                    question->text = pddby_regex_replace_literal(spaces_regex, question_text, " ");
                    free(question_text);
                    pddby_string_chomp(question->text);
                }
                break;
            case 'W':
            case 'V':
                p++;
                {
                    char** answers = pddby_regex_split(answers_regex, *p);
                    char** a = answers + 1;
                    while (*a)
                    {
                        char* answer_text = pddby_regex_replace_literal(word_break_regex, *a, "");
                        char* answer_text2 = pddby_regex_replace_literal(spaces_regex, answer_text, " ");
                        free(answer_text);
                        pddby_answer_t *answer = pddby_answer_new(context->pddby, 0, answer_text2, 0);
                        free(answer_text2);
                        pddby_string_chomp(answer->text);
                        pddby_array_add(question_answers, answer);
                        a++;
                    }
                    pddby_stringv_free(answers);
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
                    char* advice_text = pddby_regex_replace_literal(word_break_regex, *p, "");
                    question->advice = pddby_regex_replace_literal(spaces_regex, advice_text, " ");
                    free(advice_text);
                    pddby_string_chomp(question->advice);
                }
                break;
            case 'L':
                p++;
                {
                    char** traffreg_numbers = pddby_string_split(*p, " ");
                    char** trn = traffreg_numbers;
                    while (*trn)
                    {
                        pddby_traffreg_t *traffreg = pddby_traffreg_find_by_number(context->pddby, atoi(*trn));
                        if (!traffreg)
                        {
                            //pddby_report_error("unable to find traffreg with number %s\n", *trn);
                        }
                        pddby_array_add(question_traffregs, traffreg);
                        trn++;
                    }
                    pddby_stringv_free(traffreg_numbers);
                }
                break;
            case 'C':
                p++;
                {
                    pddby_comment_t *comment = pddby_comment_find_by_number(context->pddby, atoi(*p));
                    if (!comment)
                    {
                        //pddby_report_error("unable to find comment with number %s\n", *p);
                    }
                    question->comment_id = comment->id;
                    pddby_comment_free(comment);
                }
                break;
            default:
                //pddby_report_error("unknown question data section: %s\n", *p);
                ;
            }
            p++;
        }
        pddby_stringv_free(parts);

        result = pddby_question_save(question);
        if (!result)
        {
            //pddby_report_error("unable to save question\n");
        }
        for (size_t k = 0; k < pddby_array_size(question_answers); k++)
        {
            pddby_answer_t* answer = (pddby_answer_t*)pddby_array_index(question_answers, k);
            answer->question_id = question->id;
            answer->is_correct = k == answer_number;
            result = pddby_answer_save(answer);
            if (!result)
            {
                //pddby_report_error("unable to save answer\n");
            }
        }
        pddby_answers_free(question_answers);
        result = pddby_question_set_sections(question, question_sections);
        if (!result)
        {
            //pddby_report_error("unable to set question sections\n");
        }
        pddby_sections_free(question_sections);
        result = pddby_question_set_traffregs(question, question_traffregs);
        if (!result)
        {
            //pddby_report_error("unable to set question traffregs\n");
        }
        pddby_traffregs_free(question_traffregs);
        pddby_question_free(question);
        //printf(".");
    }
    pddby_db_tx_commit(context->pddby);

    //printf("\n");

    pddby_regex_free(spaces_regex);
    pddby_regex_free(word_break_regex);
    pddby_regex_free(answers_regex);
    pddby_regex_free(question_data_regex);
    free(str);

    return result;
}

int pddby_compare_topic_questions(void const* first, void const* second)
{
    pddby_topic_question_t const* first_tq = (pddby_topic_question_t const*)first;
    pddby_topic_question_t const* second_tq = (pddby_topic_question_t const*)second;

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
