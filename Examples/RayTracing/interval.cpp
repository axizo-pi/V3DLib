#include "interval.h"
#include "rtweekend.h"

interval::interval() : min(+infinity), max(-infinity) {} // Default interval is empty

const interval interval::empty    = interval(+infinity, -infinity);
const interval interval::universe = interval(-infinity, +infinity);

