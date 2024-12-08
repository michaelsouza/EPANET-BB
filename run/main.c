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
#include <stdlib.h>

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
  double min_level;
  double max_level;
} BBTank;

typedef struct {
  char id[MAXID];
  int index;  
} BBNode;

typedef struct {
  int num_pumps;
  int num_tanks;
  int prices_index;
  BBPump *pumps;
  BBTank *tanks;
  BBNode *nodes;
} BBProject;

void writeConsole(char *s) {
  fprintf(stdout, "%s\n", s);
  fflush(stdout);
}

void BB_show_pump(EN_Project p, BBPump *pump) {
  // sprintf(p->Msg, "Pump %s has index %d and pattern index %d", pump->id,
  // pump->index, pump->pattern_index); writewin(p->viewprog, p->Msg);
  printf("Pump[%3s] has index %d and pattern index %d\n", pump->id, pump->index,
         pump->pattern_index);
}

void BB_show_tank(EN_Project p, BBTank *tank) {
  printf("Tank[%3s] has index %d and level %6.2f\n", tank->id, tank->index,
         tank->level);
}

void BB_show_node(EN_Project p, BBNode *node) {
  printf("Node[%3s] has index %d\n", node->id, node->index);
}

void BB_show_header(char *title) {
  int len = strlen(title);
  int total_len = 50 - len;
  printf("%s ", title);
  for (int i = 0; i < total_len; i++) {
    printf("=");
  }
  printf("\n");
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

void BB_init(EN_Project p, int hour, int **x) {}

void BB_new(EN_Project p, BBProject *bb_prj) {
  BB_show_header("BB_new");
  // Create pumps ====================================================
  const int num_pumps = 3;
  bb_prj->num_pumps = num_pumps;
  bb_prj->pumps = (BBPump*)malloc(num_pumps * sizeof(BBPump));
  
  strncpy(bb_prj->pumps[0].id, "111", MAXID);
  strncpy(bb_prj->pumps[1].id, "222", MAXID); 
  strncpy(bb_prj->pumps[2].id, "333", MAXID);
  
  for (int i = 0; i < num_pumps; i++) {
    EN_getlinkindex(p, bb_prj->pumps[i].id, &bb_prj->pumps[i].index);
    char pattern_id[MAXID];
    sprintf(pattern_id, "PMP%s", bb_prj->pumps[i].id);
    EN_getpatternindex(p, pattern_id, &bb_prj->pumps[i].pattern_index);
    // Show pump
    BB_show_pump(p, &bb_prj->pumps[i]);
  }

  // Create tanks ====================================================
  const int num_tanks = 3;
  bb_prj->num_tanks = num_tanks;
  bb_prj->tanks = (BBTank*)malloc(num_tanks * sizeof(BBTank));
  
  strncpy(bb_prj->tanks[0].id, "65", MAXID);
  strncpy(bb_prj->tanks[1].id, "165", MAXID);
  strncpy(bb_prj->tanks[2].id, "265", MAXID);
  
  for (int i = 0; i < num_tanks; i++) {
    EN_getnodeindex(p, bb_prj->tanks[i].id, &bb_prj->tanks[i].index);
    EN_getnodevalue(p, bb_prj->tanks[i].index, EN_TANKLEVEL, &bb_prj->tanks[i].level);
    BB_show_tank(p, &bb_prj->tanks[i]);
  }

  // Create nodes ====================================================
  const int num_nodes = 3;
  bb_prj->nodes = (BBNode*)malloc(num_nodes * sizeof(BBNode));
  strncpy(bb_prj->nodes[0].id, "55", MAXID);
  strncpy(bb_prj->nodes[1].id, "90", MAXID);
  strncpy(bb_prj->nodes[2].id, "170", MAXID);
  for (int i = 0; i < num_nodes; i++) {
    EN_getnodeindex(p, bb_prj->nodes[i].id, &bb_prj->nodes[i].index);
    BB_show_node(p, &bb_prj->nodes[i]);
  }

  // Get prices =======================================================
  EN_getpatternindex(p, "PRICES", &bb_prj->prices_index);
}

void BB_free(BBProject *project) {
  free(project->pumps);
  free(project->tanks);
  free(project->nodes);
}

int BB_solveH(EN_Project p, BBProject *bb)
/*----------------------------------------------------------------
 **  Input:   none
 **  Output:  none
 **  Returns: error code
 **  Purpose: solves for network hydraulics in all time periods
 **----------------------------------------------------------------
 */
{
  int errcode = 0;
  long t, tstep;

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
      for (int i = 0; i < bb->num_tanks; i++) {
        BBTank *tank = &bb->tanks[i];
        EN_getnodevalue(p, tank->index, EN_TANKLEVEL, &tank->level);
        BB_show_tank(p, tank);
      }
      // =================================================================

      // Get pump costs ==================================================
      int hora = (int)floor(t / 3600.0);
      for (int i = 0; i < bb->num_pumps; i++) {
        BBPump *pump = &bb->pumps[i];
        double energy = 0.0, price = 0.0;
        EN_getlinkvalue(p, pump->index, EN_ENERGY, &energy);
        EN_getpatternvalue(p, bb->prices_index, hora + 1, &price);
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
  
  BBProject bb; 
  BB_new(&p, &bb);

  errcode = BB_solveH(&p, &bb);
  if (errcode) {
    printf("Error[%d] The hydraulic solver failed.", errcode);
    return errcode;
  }

  BB_free(&bb);
  EN_close(&p);
}
