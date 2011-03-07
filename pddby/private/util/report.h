#ifndef PDDBY_PRIVATE_REPORT_H
#define PDDBY_PRIVATE_REPORT_H

#include "pddby.h"

void pddby_report(pddby_t* pddby, int type, char const* text, ...);

void pddby_report_progress_begin(pddby_t* pddby, int size);
void pddby_report_progress(pddby_t* pddby, int pos);
void pddby_report_progress_end(pddby_t* pddby);

#endif // PDDBY_PRIVATE_REPORT_H
