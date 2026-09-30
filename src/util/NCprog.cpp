/*
 * NCprog.cpp â€” .iso file loader, reuses the original parser.
 *
 */
#include <fstream>
#include <iostream>
#include <cstdio>
#include "NCprog.h"

using namespace std;

NCprog::NCprog(string fn)
{
    WorkPlane   = 0;
    TotalPoints = 0;
    LoadNCprog(fn);
}

NCprog::~NCprog() {}

void NCprog::SetFileName(string fn) { fileName = fn; }

int NCprog::LoadNCprog(string fn)
{
    ifstream in(fn.c_str());
    if (!in.is_open()) {
        cerr << "FILE IS NOT OPEN: " << fn << endl;
        return -1;
    }
    cout << "FILE IS OPEN: " << fn << endl;

    char   cline[256];
    string prevLine = "START";

    Xmax = Ymax = Zmax = -1.0e30;
    Xmin = Ymin = Zmin =  1.0e30;
    TotalPoints = 0;

    while (in.getline(cline, 256)) {
        string line(cline);
        if (line.empty()) continue;
        char c0 = line.at(0);
        if (c0 == '%' || c0 == '#' || c0 == '(') continue;

        NCinstr instr;
        instr.Parse(prevLine, line);
        prevLine = instr.GetLastValues();
        prog.push_back(instr);

        if (instr.type == 'G' &&
            (instr.code == 0 || instr.code == 1 || instr.code == 2 || instr.code == 3)) {
            if (instr.X != undefDouble) { if (instr.X > Xmax) Xmax = instr.X; if (instr.X < Xmin) Xmin = instr.X; }
            if (instr.Y != undefDouble) { if (instr.Y > Ymax) Ymax = instr.Y; if (instr.Y < Ymin) Ymin = instr.Y; }
            if (instr.Z != undefDouble) { if (instr.Z > Zmax) Zmax = instr.Z; if (instr.Z < Zmin) Zmin = instr.Z; }
        }

        if (instr.type == 'G' && instr.code == 17) WorkPlane = 2;
        if (instr.type == 'G' && instr.code == 18) WorkPlane = 3;
        TotalPoints++;
    }

    cout << "End File â€” " << TotalPoints << " instructions, "
         << "bounds X[" << Xmin << ".." << Xmax << "] "
         << "Y[" << Ymin << ".." << Ymax << "] "
         << "Z[" << Zmin << ".." << Zmax << "]" << endl;
    return TotalPoints;
}

void NCprog::ShowList()
{
    for (auto& instr : prog) {
        char buff[256];
        sprintf(buff, "N%d %c%d X0=%.3f Y0=%.3f Z0=%.3f X=%.3f Y=%.3f Z=%.3f\r\n",
                instr.N, instr.type, instr.code,
                instr.X0, instr.Y0, instr.Z0,
                instr.X,  instr.Y,  instr.Z);
        cout << buff;
    }
}
