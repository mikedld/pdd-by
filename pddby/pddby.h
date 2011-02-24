#ifndef PDDBY_PDDBY_H
#define PDDBY_PDDBY_H

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct pddby pddby_t;

enum pddby_message_type
{
    pddby_message_type_debug,
    pddby_message_type_log,
    pddby_message_type_warning,
    pddby_message_type_error,
};

struct pddby_callbacks
{
    void (*message)(pddby_t* pddby, int type, char const* text);

    void (*progress_begin)(pddby_t* pddby, int size);
    void (*progress)(pddby_t* pddby, int pos);
    void (*progress_end)(pddby_t* pddby);
};

typedef struct pddby_callbacks pddby_callbacks_t;

pddby_t* pddby_init(char const* share_dir, char const* cache_dir, pddby_callbacks_t* callbacks);
void pddby_close(pddby_t* pddby);

int pddby_decode(pddby_t* pddby, char const* root_path);

int pddby_cache_exists(pddby_t* pddby);
void pddby_use_cache(pddby_t* pddby, int value);

#ifdef __cplusplus
}
#endif

#endif // PDDBY_PDDBY_H
