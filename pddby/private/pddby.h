#ifndef PDDBY_PRIVATE_PDDBY_H
#define PDDBY_PRIVATE_PDDBY_H

struct pddby_callbacks;
struct pddby_db;
struct pddby_decode_context;

struct pddby
{
    struct pddby_callbacks* callbacks;
    struct pddby_db* database;
    struct pddby_decode_context* decode_context;
};

#endif // PDDBY_PRIVATE_PDDBY_H
