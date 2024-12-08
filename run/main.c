/*
 ******************************************************************************
 Project:      OWA EPANET
 Version:      2.2
 Module:       main.c
 Description:  main stub for a command line executable version of EPANET
 Authors:      see AUTHORS
 Copyright:    see AUTHORS
 License:      see LICENSE
 Last Updated: 12/07/2018
 ******************************************************************************
*/

#include "bb.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "epanet2.h"
#include "epanet2_2.h"
#include "types.h"

typedef struct {
  char id[MAXID];
  int index;
  int pattern_index;
} BBPump;

typedef struct {
  char id[MAXID];
  int index;
  double level;
} BBTank;

void writeConsole(char *s) {
  fprintf(stdout, "\r%s", s);
  fflush(stdout);
}

void BB_show_pump(EN_Project p, BBPump *pump) {
  // sprintf(p->Msg, "Pump %s has index %d and pattern index %d", pump->id,
  // pump->index, pump->pattern_index); writewin(p->viewprog, p->Msg);
  printf("%3s has index %d and pattern index %d\n", pump->id, pump->index,
         pump->pattern_index);
}

void BB_show_tank(EN_Project p, BBTank *tank) {
  printf("%3s has index %d and level %6.2f\n", tank->id, tank->index,
         tank->level);
}

int BB_open(Project *p, const char *inpFile, const char *rptFile,
            const char *outFile) {
  // Read in project data from an input file
  int errcode;
  p->viewprog = &writeConsole;
  errcode = EN_open(p, inpFile, rptFile, outFile);
  if (errcode > 100) {
    printf("Error[%d] The project file is not valid.\n", errcode);
    printf(" inpFile: %s\n", inpFile);
    printf(" rptFile: %s\n", rptFile);
  }
  return errcode;
}

int BB_solveH(EN_Project p)
/*----------------------------------------------------------------
 **  Input:   none
 **  Output:  none
 **  Returns: error code
 **  Purpose: solves for network hydraulics in all time periods
 **----------------------------------------------------------------
 */
{
  int errcode;
  long t, tstep;

  // Create pumps ====================================================
  const int num_pumps = 3;
  BBPump pumps[num_pumps];
  strncpy(pumps[0].id, "111", MAXID);
  strncpy(pumps[1].id, "222", MAXID);
  strncpy(pumps[2].id, "333", MAXID);
  for (int i = 0; i < num_pumps; i++) {
    EN_getlinkindex(p, pumps[i].id, &pumps[i].index);
    char pattern_id[MAXID];
    sprintf(pattern_id, "PMP%s", pumps[i].id);
    EN_getpatternindex(p, pattern_id, &pumps[i].pattern_index);
    // Show pump
    BB_show_pump(p, &pumps[i]);
  }

  // Create tanks ====================================================
  const int num_tanks = 3;
  BBTank tanks[num_tanks];
  strncpy(tanks[0].id, "65", MAXID);
  strncpy(tanks[1].id, "165", MAXID);
  strncpy(tanks[2].id, "265", MAXID);
  for (int i = 0; i < num_tanks; i++) {
    EN_getnodeindex(p, tanks[i].id, &tanks[i].index);
    EN_getnodevalue(p, tanks[i].index, EN_TANKLEVEL, &tanks[i].level);
    BB_show_tank(p, &tanks[i]);
  }

  // Get prices =======================================================
  int prices_index;
  EN_getpatternindex(p, "PRICES", &prices_index);

  // Open hydraulics solver ===========================================
  EN_openH(p);
  double total_cost = 0.0;
  if (!errcode) {
    // Initialize hydraulics
    errcode = EN_initH(p, EN_SAVE);
    if (errcode) {
      printf("Error[%d] The hydraulic solver failed to initialize.", errcode);
      return errcode;
    }

    // Analyze each hydraulic time period
    tstep = 1; // Arbitrary non-zero value
    while (tstep > 0) {
      // Display progress message
      // sprintf(p->Msg, "%-10s", clocktime(p->report.Atime, p->times.Htime));
      // sprintf(p->Msg, FMT101, p->report.Atime);
      // writewin(p->viewprog, p->Msg);

      printf("t: %5ld, tstep: %5ld =============================\n", t, tstep);

      // Solve for hydraulics & advance to next time period
      tstep = 0;
      errcode = EN_runH(p, &t);
      if (errcode) {
        printf("Error[%d] The hydraulic solver failed.", errcode);
        break;
      }

      // tstep determines the length of time until the next hydraulic event
      // occurs in an extended period simulation. The current solution is valid
      // for [t, t+tstep].
      errcode = EN_nextH(p, &tstep);
      if (errcode) {
        printf("Error[%d] The hydraulic solver failed.", errcode);
        break;
      }

      // Get tank levels ==================================================
      for (int i = 0; i < num_tanks; i++) {
        BBTank *tank = &tanks[i];
        EN_getnodevalue(p, tank->index, EN_TANKLEVEL, &tank->level);
        BB_show_tank(p, tank);
      }
      // =================================================================

      // Get pump costs ==================================================
      int hora = (int)floor(t / 3600.0);
      for (int i = 0; i < num_pumps; i++) {
        BBPump *pump = &pumps[i];
        double energy = 0.0, price = 0.0;
        EN_getlinkvalue(p, pump->index, EN_ENERGY, &energy);
        EN_getpatternvalue(p, prices_index, hora + 1, &price);
        double cost = (tstep / 3600.0) * energy * price;
        total_cost += cost;
        printf("Pump[%3s] energy %6.2f price %6.2f cost %6.2f\n", pump->id,
               energy, price, cost);
      }
      printf("total_cost %6.2f\n\n", total_cost);
      // =================================================================
    }
  }

  // Close hydraulics solver
  EN_closeH(p);
  errcode = MAX(errcode, p->Warnflag);
  return errcode;
}

int main(int argc, char *argv[])
/*--------------------------------------------------------------
 **  Input:   argc    = number of command line arguments
 **           *argv[] = array of command line arguments
 **  Output:  none
 **  Purpose: main program stub for command line EPANET
 **
 **  Command line for stand-alone operation is:
 **    progname f1  f2  f3
 **  where progname = name of executable this code was compiled to,
 **  f1 = name of input file,
 **  f2 = name of report file
 **  f3 = name of binary output file (optional).
 **--------------------------------------------------------------
 */
{
  char *f1, *f2, *f3;
  char blank[] = "";
  char errmsg[256] = "";
  int errcode;
  int version;
  int major;
  int minor;
  int patch;

  // Check for proper number of command line arguments
  if (argc < 3) {
    printf("\nUsage:\n %s <input_filename> <report_filename> "
           "[<binary_filename>]\n",
           argv[0]);
    return 0;
  }

  // Get version number and display in Major.Minor.Patch format
  ENgetversion(&version);
  major = version / 10000;
  minor = (version % 10000) / 100;
  patch = version % 100;
  printf("\n... Running EPANET Version %d.%d.%d\n", major, minor, patch);

  // Assign pointers to file names
  f1 = argv[1];
  f2 = argv[2];
  if (argc > 3)
    f3 = argv[3];
  else
    f3 = blank;

  Project p;
  BB_open(&p, f1, f2, f3);
  BB_solveH(&p);
  EN_close(&p);
}
