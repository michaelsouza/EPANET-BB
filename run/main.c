#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "epanet2_2.h"
#include "types.h"

#include "bb.h"

void writeConsole(char *s) {
  fprintf(stdout, "%s\n", s);
  fflush(stdout);
}

void BB_show_pump(BBPump *pump) {
  printf("Pump[%3s] has index %d and pattern index %d\n", pump->id, pump->index, pump->pattern_index);
}

void BB_show_tank(BBTank *tank) {
  printf("Tank[%3s] has index %d and level %6.2f\n", tank->id, tank->index, tank->level[0]);
}

void BB_show_node(BBNode *node) { printf("Node[%3s] has index %d\n", node->id, node->index); }

void BB_show_section_title(char *title) {
  int len = strlen(title);
  int total_len = 50 - len;
  printf("%s ", title);
  for (int i = 0; i < total_len; i++) {
    printf("=");
  }
  printf("\n");
}

int BB_load(Project *p, const char *inpFile, const char *rptFile, const char *outFile) {
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

void BB_init01H(EN_Project p, BBData *bb, int hour) {
  BB_show_section_title("BB_init01H");

  // Set time parameters (start time, duration)
  bb->hour = hour;
  EN_settimeparam(p, EN_STARTTIME, (hour - 1) * 3600); // start time in seconds
  EN_settimeparam(p, EN_DURATION, 3600);               // duration in seconds

  // Set initial water levels for each tank using previous hour's levels
  if (hour > 0) {
    for (int i = 0; i < bb->num_tanks; i++) {
      BBTank *tank = &bb->tanks[i];
      const double level = tank->level[hour - 1]; // previous hour's level
      EN_setnodevalue(p, tank->index, EN_TANKLEVEL, level);
    }
  }

  // Config pumps (pattern:speed)
  for (int i = 0; i < bb->num_pumps; i++) {
    BBPump *pump = &bb->pumps[i];
    int period = hour - 1;
    EN_setpatternvalue(p, pump->pattern_index, period, pump->pattern_values[period]);
  }
}

void BB_new(BBData *bb, char *inpFile, char *rptFile, char *outFile, int num_hours) {
  BB_show_section_title("BB_new");

  int errcode;
  Project p;
  BB_load(&p, inpFile, rptFile, outFile);

  bb->num_hours = num_hours;

  // Create pumps ====================================================
  const int num_pumps = 3;
  bb->num_pumps = num_pumps;
  bb->pumps = (BBPump *)malloc(num_pumps * sizeof(BBPump));

  strncpy(bb->pumps[0].id, "111", MAXID);
  strncpy(bb->pumps[1].id, "222", MAXID);
  strncpy(bb->pumps[2].id, "333", MAXID);

  for (int i = 0; i < num_pumps; i++) {
    BBPump *pump = &bb->pumps[i];
    // Get pump index
    EN_getlinkindex(&p, pump->id, &pump->index);
    // Get pattern index
    char pattern_id[MAXID];
    snprintf(pattern_id, MAXID, "PMP%.27s", pump->id);
    EN_getpatternindex(&p, pattern_id, &pump->pattern_index);
    // Allocate memory for pattern values
    pump->pattern_values = (double *)malloc(num_hours * sizeof(double));
    for (int period = 0; period < num_hours; period++) {
      EN_getpatternvalue(&p, pump->pattern_index, period, &pump->pattern_values[period]);
    }
    // Show pump
    BB_show_pump(pump);
  }

  // Create tanks ====================================================
  const int num_tanks = 3;
  bb->num_tanks = num_tanks;
  bb->tanks = (BBTank *)malloc(num_tanks * sizeof(BBTank));

  strncpy(bb->tanks[0].id, "65", MAXID);
  strncpy(bb->tanks[1].id, "165", MAXID);
  strncpy(bb->tanks[2].id, "265", MAXID);

  for (int i = 0; i < num_tanks; i++) {
    BBTank *tank = &bb->tanks[i];
    // allocate memory for tank level
    tank->level = (double *)malloc(num_hours * sizeof(double));
    EN_getnodeindex(&p, tank->id, &tank->index);
    EN_getnodevalue(&p, tank->index, EN_TANKLEVEL, &tank->level[0]);
    BB_show_tank(tank);
  }

  // Create nodes ====================================================
  const int num_nodes = 3;
  bb->nodes = (BBNode *)malloc(num_nodes * sizeof(BBNode));
  strncpy(bb->nodes[0].id, "55", MAXID);
  strncpy(bb->nodes[1].id, "90", MAXID);
  strncpy(bb->nodes[2].id, "170", MAXID);
  for (int i = 0; i < num_nodes; i++) {
    BBNode *node = &bb->nodes[i];
    EN_getnodeindex(&p, node->id, &node->index);
    BB_show_node(node);
  }

  // Get prices =======================================================
  EN_getpatternindex(&p, "PRICES", &bb->prices_index);

  EN_close(&p);
}

void BB_free(BBData *bb) {
  for (int i = 0; i < bb->num_tanks; i++) {
    free(bb->tanks[i].level);
  }
  for (int i = 0; i < bb->num_pumps; i++) {
    free(bb->pumps[i].pattern_values);
  }
  free(bb->pumps);
  free(bb->tanks);
  free(bb->nodes);
}

void BB_update(Project *p, BBData *bb) {
  BB_show_section_title("BB_update");

  // Copy values from the project "p" to the BBData "bb"
  for (int i = 0; i < bb->num_tanks; i++) {
    BBTank *tank = &bb->tanks[i];
    EN_getnodevalue(p, tank->index, EN_TANKLEVEL, &tank->level[bb->hour]);
    BB_show_tank(tank);
  }
}

int BB_solveH(Project *p, BBData *bb) {
  int errcode = 0;
  long t = 0, tstep;

  // Open hydraulics solver ===========================================
  EN_openH(p);
  double total_cost = 0.0;
  if (!errcode) {
    // Initialize hydraulics
    errcode = EN_initH(p, EN_NOSAVE);
    if (errcode) {
      printf("Error[%d] The hydraulic solver failed to initialize.", errcode);
      return errcode;
    }

    // Analyze each hydraulic time period
    tstep = 1; // Arbitrary non-zero value, just to start
    while (tstep > 0) {
      // Display progress message
      // sprintf(p->Msg, "%-10s", clocktime(p->report.Atime, p->times.Htime));
      // sprintf(p->Msg, FMT101, p->report.Atime);
      // writewin(p->viewprog, p->Msg);

      BB_show_section_title("BB_solveH: iteration");
      printf("t: %5ld, tstep: %5ld\n", t, tstep);

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
        double level;
        EN_getnodevalue(p, tank->index, EN_TANKLEVEL, &level);
        printf("Tank[%3s] level %6.2f\n", tank->id, level);
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
        printf("Pump[%3s] energy %6.2f price %6.2f cost %6.2f\n", pump->id, energy, price, cost);
      }
      printf("total_cost %6.2f\n\n", total_cost);
      // =================================================================
    }
  }

  // Update tank levels
  BB_update(p, bb);

  // Close hydraulics solver
  EN_closeH(p);
  errcode = MAX(errcode, p->Warnflag);
  return errcode;
}

int main(int argc, char *argv[]) {
  char *inpFile, *rptFile, *outFile;
  char blank[] = "";
  char errmsg[256] = "";
  int errcode = 0;
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
  EN_getversion(&version);
  major = version / 10000;
  minor = (version % 10000) / 100;
  patch = version % 100;
  printf("\n... Running EPANET Version %d.%d.%d\n", major, minor, patch);

  // Assign pointers to file names
  inpFile = argv[1];
  rptFile = argv[2];
  if (argc > 3)
    outFile = argv[3];
  else
    outFile = blank;

  BBData bb;
  BB_new(&bb, inpFile, rptFile, outFile, 24);

  Project p;
  BB_load(&p, inpFile, rptFile, outFile);
  BB_solveH(&p, &bb);
  if (errcode) {
    printf("Error[%d] The hydraulic solver failed.", errcode);
    return errcode;
  }
  EN_close(&p);

  for (int hour = 0; hour < 24; ++hour) {
    Project p;
    BB_load(&p, inpFile, rptFile, outFile);
    BB_init01H(&p, &bb, hour);
    BB_solveH(&p, &bb);
    BB_update(&p, &bb);
    EN_close(&p);
  }

  BB_free(&bb);
}
