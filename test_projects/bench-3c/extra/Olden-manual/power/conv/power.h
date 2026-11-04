/* For copyright information, see olden_v1.0/COPYRIGHT */

/*
 * power.h
 *
 * By:  Martin C. Carlisle
 * 6/15/94
 *
 * Header file for the Power Pricing problem
 * adapted from code by Steve Lumetta, Sherry Li, and Ismail Khalil
 * University of California at Berkeley
 *
 */

typedef struct demand {
  double P;
  double Q;
} Demand;

#include <math.h>


#ifdef SMALL_PROBLEM_SIZE   /* __llvm__ SCALED BACK SETTINGS */
#define NUM_FEEDERS 8
#define LATERALS_PER_FEEDER 16
#define BRANCHES_PER_LATERAL 5
#define LEAVES_PER_BRANCH 10
#else
#if 0  /* DEFAULT SETTINGS */
#define NUM_FEEDERS 10
#define LATERALS_PER_FEEDER 20
#define BRANCHES_PER_LATERAL 5
#define LEAVES_PER_BRANCH 10
#else  /* SCALED UP SETTINGS */
#define NUM_FEEDERS 11
#define LATERALS_PER_FEEDER 21
#define BRANCHES_PER_LATERAL 6
#define LEAVES_PER_BRANCH 12
#endif
#endif

#define F_EPSILON 0.000001
#define G_EPSILON 0.000001
#define H_EPSILON 0.000001
#define ROOT_EPSILON 0.00001

struct root {
  Demand D;
  double theta_R; 
  double theta_I; 
  Demand last;
  double last_theta_R; 
  double last_theta_I;
  struct lateral * feeders [NUM_FEEDERS];
};  /* sizeof(struct root) = 108 bytes */

typedef struct root * Root;

struct lateral {
  Demand D;
  double alpha;
  double beta;
  double R;
  double X;
  struct lateral * next_lateral;
  struct branch * branch;
}; /* sizeof(struct lateral) = 64 bytes */

typedef struct lateral * Lateral;

struct branch {
  Demand D;
  double alpha;
  double beta;
  double R;
  double X;
  struct branch * next_branch;
  struct leaf * leaves [LEAVES_PER_BRANCH];
}; /* sizeof(struct branch) = 92 bytes */

typedef struct branch * Branch;

struct leaf {
  Demand D;
  double pi_R;
  double pi_I;
};  /* sizeof(struct leaf) = 32 bytes */

typedef struct leaf * Leaf;

/* Prototypes */
Root build_tree(void);
Lateral build_lateral(int i, int num);
Branch build_branch(int i, int j, int num);
Leaf build_leaf(void);

void Compute_Tree(Root r);
Demand Compute_Lateral(Lateral l, double theta_R, double theta_I,
                       double pi_R, double pi_I);
Demand Compute_Branch(Branch b, double theta_R, double theta_I,
                       double pi_R, double pi_I);
Demand Compute_Leaf(Leaf l, double pi_R, double pi_I);

