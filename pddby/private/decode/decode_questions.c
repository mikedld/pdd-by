#include "decode_questions.h"

#include "decode_context.h"

#include "answer.h"
#include "comment.h"
#include "config.h"
#include "image.h"
#include "private/pddby.h"
#include "private/platform.h"
#include "private/util/aux.h"
#include "private/util/database.h"
#include "private/util/report.h"
#include "private/util/regex.h"
#include "question.h"
#include "section.h"
#include "traffreg.h"

#include <stdlib.h>

pddby_topic_question_t* pddby_decode_topic_questions_table(pddby_decode_context_t* context, char const* path, size_t* table_size)
{
    pddby_topic_question_t* table = NULL;

    char* t = NULL;
    if (!pddby_aux_file_get_contents(context->pddby, path, &t, table_size))
    {
        goto error;
    }

    table = (pddby_topic_question_t*)t;

    if (*table_size % sizeof(pddby_topic_question_t))
    {
        pddby_report(context->pddby, pddby_message_type_error, "invalid file size: %s (should be multiple of %ld)",
            path, sizeof(pddby_topic_question_t));
        goto error;
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

error:
    pddby_report(context->pddby, pddby_message_type_error, "unable to decode topic questions table");

    if (table)
    {
        free(table);
    }

    return NULL;
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

    pddby_regex_t* question_data_regex = NULL;
    pddby_regex_t* answers_regex = NULL;
    pddby_regex_t* word_break_regex = NULL;
    pddby_regex_t* spaces_regex = NULL;
    char* str = NULL;

    size_t str_size;
    str = context->pddby->decode_context->decode_string(context, dbt_path, &str_size, topic_number);
    if (!str)
    {
        goto error;
    }

    question_data_regex = pddby_regex_new(context->pddby, "\\s*\\[|\\]\\s*", PDDBY_REGEX_NEWLINE_ANY);
    if (!question_data_regex)
    {
        goto error;
    }

    answers_regex = pddby_regex_new(context->pddby, "^($^$)?\\d\\.?\\s+", PDDBY_REGEX_MULTILINE |
        PDDBY_REGEX_NEWLINE_ANY);
    if (!answers_regex)
    {
        goto error;
    }

    word_break_regex = pddby_regex_new(context->pddby, "(?<!\\s)-\\s+", PDDBY_REGEX_NEWLINE_ANY);
    if (!word_break_regex)
    {
        goto error;
    }

    spaces_regex = pddby_regex_new(context->pddby, "\\s{2,}", PDDBY_REGEX_NEWLINE_ANY);
    if (!spaces_regex)
    {
        goto error;
    }

    if (!pddby_db_tx_begin(context->pddby))
    {
        goto error;
    }

    pddby_report_progress_begin(context->pddby, table_size);

    int result = 1;
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
            goto error;
        }

        pddby_question_t* question = NULL;
        pddby_traffregs_t* question_traffregs = NULL;
        pddby_sections_t* question_sections = NULL;
        pddby_answers_t* question_answers = NULL;
        char** parts = NULL;

        parts = pddby_regex_split(question_data_regex, text);
        free(text);
        if (!parts)
        {
            goto cycle_error;
        }

        question = pddby_question_new(context->pddby, topic_number, NULL, 0, NULL, 0);
        if (!question)
        {
            goto cycle_error;
        }

        question_traffregs = pddby_traffregs_new(context->pddby);
        if (!question_traffregs)
        {
            goto cycle_error;
        }

        question_sections = pddby_sections_new(context->pddby);
        if (!question_sections)
        {
            goto cycle_error;
        }

        question_answers = pddby_answers_new(context->pddby);
        if (!question_answers)
        {
            goto cycle_error;
        }

        size_t answer_number = 0;
        char** p = parts + 1;
        while (*p)
        {
            switch ((*p)[0])
            {
            case 'R':
                p++;
                {
                    char** section_names = pddby_string_split(context->pddby, *p, " ");
                    if (!section_names)
                    {
                        goto cycle_error;
                    }

                    char** sn = section_names;
                    while (*sn)
                    {
                        pddby_section_t *section = pddby_section_find_by_name(context->pddby, *sn);
                        if (!section)
                        {
                            pddby_stringv_free(section_names);
                            goto cycle_error;
                        }

                        if (!pddby_array_add(question_sections, section))
                        {
                            pddby_stringv_free(section_names);
                            goto cycle_error;
                        }

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
                        goto cycle_error;
                    }
                    question->image_id = image->id;
                    pddby_image_free(image);
                }
                break;

            case 'Q':
                p++;
                {
                    char* question_text = pddby_regex_replace_literal(word_break_regex, *p, "");
                    if (!question_text)
                    {
                        goto cycle_error;
                    }
    
                    question->text = pddby_regex_replace_literal(spaces_regex, question_text, " ");
                    free(question_text);
                    if (!question->text)
                    {
                        goto cycle_error;
                    }

                    pddby_string_chomp(question->text);
                }
                break;

            case 'W':
            case 'V':
                p++;
                {
                    char** answers = pddby_regex_split(answers_regex, *p);
                    if (!answers)
                    {
                        goto cycle_error;
                    }

                    char** a = answers + 1;
                    while (*a)
                    {
                        char* answer_text = pddby_regex_replace_literal(word_break_regex, *a, "");
                        if (!answer_text)
                        {
                            pddby_stringv_free(answers);
                            goto cycle_error;
                        }

                        char* answer_text2 = pddby_regex_replace_literal(spaces_regex, answer_text, " ");
                        free(answer_text);
                        if (!answer_text2)
                        {
                            pddby_stringv_free(answers);
                            goto cycle_error;
                        }

                        pddby_answer_t *answer = pddby_answer_new(context->pddby, 0, answer_text2, 0);
                        free(answer_text2);
                        if (!answer)
                        {
                            pddby_stringv_free(answers);
                            goto cycle_error;
                        }

                        pddby_string_chomp(answer->text);
                        if (!pddby_array_add(question_answers, answer))
                        {
                            pddby_answer_free(answer);
                            pddby_stringv_free(answers);
                            goto cycle_error;
                        }

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
                    if (!advice_text)
                    {
                        goto cycle_error;
                    }

                    question->advice = pddby_regex_replace_literal(spaces_regex, advice_text, " ");
                    free(advice_text);
                    if (!question->advice)
                    {
                        goto cycle_error;
                    }

                    pddby_string_chomp(question->advice);
                }
                break;

            case 'L':
                p++;
                {
                    char** traffreg_numbers = pddby_string_split(context->pddby, *p, " ");
                    if (!traffreg_numbers)
                    {
                        goto cycle_error;
                    }

                    char** trn = traffreg_numbers;
                    while (*trn)
                    {
                        pddby_traffreg_t *traffreg = pddby_traffreg_find_by_number(context->pddby, atoi(*trn));
                        if (!traffreg)
                        {
                            pddby_stringv_free(traffreg_numbers);
                            goto cycle_error;
                        }

                        if (!pddby_array_add(question_traffregs, traffreg))
                        {
                            pddby_stringv_free(traffreg_numbers);
                            goto cycle_error;
                        }

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
                        goto cycle_error;
                    }

                    question->comment_id = comment->id;
                    pddby_comment_free(comment);
                }
                break;

            default:
                pddby_report(context->pddby, pddby_message_type_error, "unknown question data section: %s", *p);
                goto cycle_error;
            }
            p++;
        }

        if (!pddby_question_save(question))
        {
            goto cycle_error;
        }
        for (size_t k = 0, size = pddby_array_size(question_answers); k < size; k++)
        {
            pddby_answer_t* answer = (pddby_answer_t*)pddby_array_index(question_answers, k);
            answer->question_id = question->id;
            answer->is_correct = k == answer_number;
            if (!pddby_answer_save(answer))
            {
                goto cycle_error;
            }
        }
        if (!pddby_question_set_sections(question, question_sections))
        {
            goto cycle_error;
        }
        if (!pddby_question_set_traffregs(question, question_traffregs))
        {
            goto cycle_error;
        }

        pddby_stringv_free(parts);
        pddby_answers_free(question_answers);
        pddby_sections_free(question_sections);
        pddby_traffregs_free(question_traffregs);
        pddby_question_free(question);

        pddby_report_progress(context->pddby, i);
        continue;

cycle_error:
        pddby_report(context->pddby, pddby_message_type_error, "unable to decode question #%lu of topic #%d", i,
            topic_number);

        if (parts)
        {
            pddby_stringv_free(parts);
        }
        if (question_answers)
        {
            pddby_answers_free(question_answers);
        }
        if (question_sections)
        {
            pddby_sections_free(question_sections);
        }
        if (question_traffregs)
        {
            pddby_traffregs_free(question_traffregs);
        }
        if (question)
        {
            pddby_question_free(question);
        }

        goto error;
    }

    pddby_report_progress_end(context->pddby);

    if (!pddby_db_tx_commit(context->pddby))
    {
        goto error;
    }

    free(str);
    pddby_regex_free(spaces_regex);
    pddby_regex_free(word_break_regex);
    pddby_regex_free(answers_regex);
    pddby_regex_free(question_data_regex);

    return result;

error:
    pddby_report(context->pddby, pddby_message_type_error, "unable to decode questions data");

    if (str)
    {
        free(str);
    }
    if (spaces_regex)
    {
        pddby_regex_free(spaces_regex);
    }
    if (word_break_regex)
    {
        pddby_regex_free(word_break_regex);
    }
    if (answers_regex)
    {
        pddby_regex_free(answers_regex);
    }
    if (question_data_regex)
    {
        pddby_regex_free(question_data_regex);
    }

    return 0;
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
