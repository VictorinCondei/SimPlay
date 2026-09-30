/*
 * NCinstr.h â€” single NC instruction (one G/M-code line) with parameter parser.
 *
 */
#ifndef UTIL_NCINSTR_H_
#define UTIL_NCINSTR_H_

#include <string>

#define TESTCAR(c) ((c) >= 'A' && (c) <= 'Z') ? 1 : 0

// Shared constants â€” defined once here, declared extern so callers can read.
extern const int    undefInt;
extern const double  undefDouble;

class NCinstr
{
public:
    bool    vacant;
    char    type;            // 'G' or 'M' (or '$' before parsing)
    int     code;            // G/M number
    char    lastValues[250];

    double  X, Y, Z, I, J, K, R, A, B, C;
    double  X0, Y0, Z0, A0, B0, C0, R0;     // values carried from the previous line
    int     N, Rp, T, S, F, H, D, P;

    NCinstr();
    ~NCinstr();

    // Parse a single NC line, merging modal values from the previous line.
    void  Parse(std::string lastLine, std::string currLine);
    void  AttachValue(std::string line, char param, bool last);
    char* GetLastValues();
};

#endif /* UTIL_NCINSTR_H_ */
