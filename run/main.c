// run/main.c

#include "epanet2_2.h"
#include "types.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_PUMPS 3 // We have 3 pumps: "55", "90", "170"
#define MAX_HOURS 24

static int pumpLinkIds[NUM_PUMPS];
static double bestEnergyCost = 1.0e12; // large initial value

// Function prototypes
int initializeSystem(EN_Project ph, const char *inpFile);
int closeSystem(EN_Project ph);
int runHour(EN_Project ph, int hourIndex, int pumpStateCombo, double prevCost);
void branchAndBound(EN_Project ph, int hourIndex, double prevCost);
double calculateHourEnergyCost(EN_Project ph, int hourIndex);
void setPumpStates(EN_Project ph, int combo);
void viewer(char *s);

//-------------------------------------------------------------------
// Main
//-------------------------------------------------------------------
int main(void) {
  int err;
  Project p;
  EN_Project ph = &p;
  ph->viewprog = viewer; // set project viewer

  // 1) Initialize system
  err = initializeSystem(ph, "/home/michael/gitrepos/EPANET-BB/example-networks/any-town.inp");
  if (err) {
    printf("Initialization error, code = %d\n", err);
    return -1;
  }

  // 2) We’ll solve hour 0 for each possible pump combination,
  //    then recursively branch & bound for hours [1..23].
  for (int combo = 0; combo < 8; combo++) {
    printf("\n--- Starting hour 0 run with combo = %d ---\n", combo);
    // Start cost is 0 for the new branch
    runHour(ph, 0, combo, 0.0);

    double startCost = calculateHourEnergyCost(ph, 0);
    printf("Hour 0 cost with combo %d = %f\n", combo, startCost);

    // Now branch from hour 1 onward
    branchAndBound(ph, 1, startCost);
  }

  // 3) Close system
  closeSystem(ph);

  printf("\nBest total energy cost found = %lf\n", bestEnergyCost);
  return EXIT_SUCCESS;
}

// Viewer function to display messages
void viewer(char *s) {
  if (s) {
    printf("%s\n", s);
  }
}

//-------------------------------------------------------------------
// initializeSystem:
//   - opens EPANET project
//   - maps pump IDs to indices
//-------------------------------------------------------------------
int initializeSystem(EN_Project ph, const char *inpFile) {
  int err;
  char rptFile[256];
  char outFile[256];

  snprintf(rptFile, sizeof(rptFile), "%s", inpFile);
  snprintf(outFile, sizeof(outFile), "%s", inpFile);

  char *ext = strrchr(rptFile, '.');
  if (ext) {
    strcpy(ext, ".rpt");
  }
  ext = strrchr(outFile, '.');
  if (ext) {
    strcpy(ext, ".out");
  }

  err = EN_open(ph, inpFile, rptFile, outFile);
  if (err) {
    printf("Error opening EPANET project, code = %d\n", err);
    return err;
  }

  // Retrieve pump indices
  err = EN_getlinkindex(ph, "111", &pumpLinkIds[0]);
  if (err)
    return err;
  err = EN_getlinkindex(ph, "222", &pumpLinkIds[1]);
  if (err)
    return err;
  err = EN_getlinkindex(ph, "333", &pumpLinkIds[2]);
  if (err)
    return err;

  printf("Initialization successful. Pump indices: 111 -> %d, 222 -> %d, 333 -> %d\n", pumpLinkIds[0], pumpLinkIds[1],
         pumpLinkIds[2]);

  return 0;
}

//-------------------------------------------------------------------
// closeSystem:
//   finalize usage of EPANET
//-------------------------------------------------------------------
int closeSystem(EN_Project ph) {
  EN_close(ph);
  printf("System closed.\n");
  return 0;
}

//-------------------------------------------------------------------
// runHour:
//   - loads hydraulics file from hourIndex-1
//   - sets pump states from combo
//   - runs single hour
//   - saves .hyd file for hourIndex
//-------------------------------------------------------------------
int runHour(EN_Project ph, int hourIndex, int pumpStateCombo, double prevCost) {
  int err = 0;
  long t = 0, dt = 0;

  // Set simulation start and end time based on hourIndex
  long startTime = hourIndex * 3600;     // Convert hours to seconds
  long endTime = (hourIndex + 1) * 3600; // 1 hour duration in seconds

  printf("\n[runHour] hourIndex = %d, combo = %d, prevCost = %f\n", hourIndex, pumpStateCombo, prevCost);
  printf("Setting startTime=%ld, endTime=%ld\n", startTime, endTime);

  err = EN_settimeparam(ph, EN_STARTTIME, startTime);
  if (err) {
    printf("Error setting simulation start time for hour %d. Code = %d\n", hourIndex, err);
    return err;
  }

  err = EN_settimeparam(ph, EN_DURATION, endTime - startTime);
  if (err) {
    printf("Error setting simulation duration for hour %d. Code = %d\n", hourIndex, err);
    return err;
  }

  err = EN_settimeparam(ph, EN_REPORTSTART, startTime);
  if (err) {
    printf("Error setting simulation start time for hour %d. Code = %d\n", hourIndex, err);
    return err;
  }

  if (hourIndex > 0) {
    char prevFile[64];
    sprintf(prevFile, "hour%d.hyd", hourIndex - 1);

    err = EN_usehydfile(ph, prevFile);
    if (err) {
      printf("Error using previous .hyd file (%s). Code = %d\n", prevFile, err);
      return err;
    }

    err = EN_openH(ph);
    if (err) {
      printf("Error opening hydraulics in read mode for hour %d. Code = %d\n", hourIndex, err);
      return err;
    }
    EN_closeH(ph);
  }

  // Open in write mode
  err = EN_openH(ph);
  if (err) {
    printf("Error opening hydraulics in write mode for hour %d. Code = %d\n", hourIndex, err);
    return err;
  }

  err = EN_initH(ph, EN_SAVE_AND_INIT);
  if (err) {
    printf("Error initH for hour %d. Code = %d\n", hourIndex, err);
    EN_closeH(ph);
    return err;
  }

  // Now set the pumps
  setPumpStates(ph, pumpStateCombo);

  dt = 1; // any nonzero value will work
  while (dt > 0) {
    err = EN_runH(ph, &t);
    if (err) {
      printf("Error running hydraulics for hour %d. Code = %d\n", hourIndex, err);
      EN_closeH(ph);
      return err;
    }
    err = EN_nextH(ph, &dt);
    if (err) {
      printf("Error getting next hydraulic time step for hour %d. Code = %d\n", hourIndex, err);
      EN_closeH(ph);
      return err;
    }
  }
  EN_closeH(ph);

  char hydFile[64];
  sprintf(hydFile, "hour%d.hyd", hourIndex);

  printf("Saving .hyd file: %s\n", hydFile);
  printf("ph->outfile.Hydflag = %d\n", ph->outfile.Hydflag);
  printf("ph->outfile.HydFname = %s\n", ph->outfile.HydFname);
  printf("ph->outfile.HydFile = %s\n", ph->outfile.HydFile == NULL ? "NULL" : "NOT NULL");
  printf("ph->outfile.SaveHflag = %d\n", ph->outfile.SaveHflag);

  err = EN_savehydfile(ph, hydFile);
  if (err) {
    printf("Error saving hydraulics file for hour %d. Code = %d\n", hourIndex, err);
    EN_closeH(ph);
    return err;
  }
  printf("[runHour] Saved .hyd file: %s\n", hydFile);

  EN_closeH(ph);
  return 0;
}

//-------------------------------------------------------------------
// branchAndBound:
//   - recursion from hourIndex to hourIndex+1
//-------------------------------------------------------------------
void branchAndBound(EN_Project ph, int hourIndex, double prevCost) {
  if (hourIndex >= MAX_HOURS) {
    // We finished 24 hours
    if (prevCost < bestEnergyCost) {
      bestEnergyCost = prevCost;
      printf("[branchAndBound] Found a new best cost at hour %d -> %f\n", hourIndex, bestEnergyCost);
    }
    return;
  }

  for (int combo = 0; combo < 8; combo++) {
    int err = runHour(ph, hourIndex, combo, prevCost);
    if (err) {
      printf("[branchAndBound] Error in runHour for hour=%d, combo=%d.\n", hourIndex, combo);
      continue;
    }

    double hourCost = calculateHourEnergyCost(ph, hourIndex);
    double totalCost = prevCost + hourCost;

    printf("[branchAndBound] hourIndex=%d, combo=%d, hourCost=%f, totalCost=%f\n", hourIndex, combo, hourCost,
           totalCost);

    if (totalCost >= bestEnergyCost) {
      // Prune
      // You can print a message if you want to see pruning in action:
      // printf("[branchAndBound] Pruning branch: cost %f >= bestKnown %f\n", totalCost, bestEnergyCost);
      continue;
    }

    branchAndBound(ph, hourIndex + 1, totalCost);
  }
}

//-------------------------------------------------------------------
// calculateHourEnergyCost:
//   - simplistic placeholder for real energy cost
//-------------------------------------------------------------------
double calculateHourEnergyCost(EN_Project ph, int hourIndex) {
  int err;
  double totalCost = 0.0;
  char hydFile[64];
  sprintf(hydFile, "hour%d.hyd", hourIndex);

  err = EN_usehydfile(ph, hydFile);
  if (err) {
    printf("[calculateHourEnergyCost] Error reading .hyd file %s, code = %d\n", hydFile, err);
    return 1.0e12;
  }

  err = EN_openH(ph);
  if (err) {
    printf("[calculateHourEnergyCost] Error opening hydraulics in read mode for hour %d, code=%d\n", hourIndex, err);
    return 1.0e12;
  }

  // For each pump, check if it’s ON
  for (int i = 0; i < NUM_PUMPS; i++) {
    double setting = 0.0;
    err = EN_getlinkvalue(ph, pumpLinkIds[i], EN_SETTING, &setting);
    if (!err && setting > 0.5) {
      totalCost += 10.0; // placeholder
    }
  }

  EN_closeH(ph);
  printf("[calculateHourEnergyCost] hourIndex=%d, computed cost=%f\n", hourIndex, totalCost);
  return totalCost;
}

//-------------------------------------------------------------------
// setPumpStates:
//   - interprets 'combo' as 3 bits and sets each pump ON/OFF
//-------------------------------------------------------------------
void setPumpStates(EN_Project ph, int combo) {
  for (int p = 0; p < NUM_PUMPS; p++) {
    int bitVal = (combo >> p) & 1;
    double setting = (bitVal == 1) ? 1.0 : 0.0;
    EN_setlinkvalue(ph, pumpLinkIds[p], EN_SETTING, setting);

    printf("[setPumpStates] Pump %d (ID index %d): %s\n", p, pumpLinkIds[p], (bitVal == 1 ? "ON" : "OFF"));
  }
}
