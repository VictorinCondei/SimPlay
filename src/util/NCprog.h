/*
 * NCprog.h â€” collection of NCinstr parsed from a .iso / .nc / .gcode file.
 */
#ifndef UTIL_NCPROG_H_
#define UTIL_NCPROG_H_

#include <list>
#include <string>
#include "NCinstr.h"

class NCprog
{
public:
    int                 TotalPoints;
    std::string         fileName;
    std::list<NCinstr>  prog;
    double Xmax, Ymax, Zmax, Xmin, Ymin, Zmin;
    int    WorkPlane;          // 0=unknown, 2=G17 (XY), 3=G18 (XZ)

    NCprog(std::string fileName);
    ~NCprog();

    void SetFileName(std::string fileName);
    int  LoadNCprog(std::string fileName);
    void ShowList();
};

#endif /* UTIL_NCPROG_H_ */
