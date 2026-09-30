/*
 * NCinstr.cpp
 *
 */
#include "NCinstr.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>

using namespace std;

const int    undefInt    = 101010;
const double undefDouble = (double)101010;

NCinstr::NCinstr()
{
    vacant = true;
    type   = '$';
    code   = undefInt;
    N      = undefInt;
    X0 = Y0 = Z0 = A0 = B0 = C0 = undefDouble;
    X  = Y  = Z  = I  = J  = K  = undefDouble;
    A  = B  = C  = R  = R0     = undefDouble;
    Rp = T = S = F = H = D = P = undefInt;
}

NCinstr::~NCinstr() {}

void NCinstr::Parse(string lastLine, string currLine)
{
    string toFind("NGMSFTHDPXYZIJKRABC");

    if (lastLine != "START") {
        vacant = false;
        transform(lastLine.begin(), lastLine.end(), lastLine.begin(), ::toupper);
        for (size_t i = 0; i < toFind.size(); i++)
            AttachValue(lastLine, toFind[i], true);
    }

    transform(currLine.begin(), currLine.end(), currLine.begin(), ::toupper);
    for (size_t i = 0; i < toFind.size(); i++)
        AttachValue(currLine, toFind[i], false);

    sprintf(lastValues, "N%dG%dT%dS%dF%dX%fY%fZ%fA%fB%fC%f",
            N, code, T, S, F, X, Y, Z, A, B, C);
}

void NCinstr::AttachValue(string line, char param, bool last)
{
    char strVal[20];
    int poz = (int)line.find(param);
    int j   = 0;

    if (poz > -1) {
        for (int i = poz + 1; i < (int)line.size(); i++) {
            if (!TESTCAR(line[i]))
                strVal[j++] = line[i];
            else
                break;
        }
        strVal[j] = '\0';

        switch (param) {
            case 'N': N    = atoi(strVal); break;
            case 'G': type = 'G'; code = atoi(strVal); break;
            case 'M': type = 'M'; code = atoi(strVal); break;
            case 'S': S    = atoi(strVal); break;
            case 'F': F    = atoi(strVal); break;
            case 'T': T    = atoi(strVal); break;
            case 'H': H    = atoi(strVal); break;
            case 'D': D    = atoi(strVal); break;
            case 'P': P    = atoi(strVal); break;
            case 'X': if (last) X0 = X = atof(strVal); else X = atof(strVal); break;
            case 'Y': if (last) Y0 = Y = atof(strVal); else Y = atof(strVal); break;
            case 'Z': if (last) Z0 = Z = atof(strVal); else Z = atof(strVal); break;
            case 'I': I = atof(strVal); break;
            case 'J': J = atof(strVal); break;
            case 'K': K = atof(strVal); break;
            case 'A': if (last) A0 = A = atof(strVal); else A = atof(strVal); break;
            case 'B': if (last) B0 = B = atof(strVal); else B = atof(strVal); break;
            case 'C': if (last) C0 = C = atof(strVal); else C = atof(strVal); break;
            case 'R': if (last) R0 = R = atof(strVal); else R = atof(strVal); break;
        }
    }
}

char* NCinstr::GetLastValues() { return lastValues; }
