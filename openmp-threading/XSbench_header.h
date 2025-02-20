#ifndef __XSBENCH_HEADER_H__
#define __XSBENCH_HEADER_H__

#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<string.h>
#include<math.h>
#include<assert.h>
#include<stdint.h>

// osm
#include <unistd.h>
#include <arpa/inet.h>

#ifdef _MSC_VER
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#else
#include<unistd.h>
#include<sys/time.h>
#endif

#ifdef OPENMP
#include<omp.h>
#endif

// Papi Header
#ifdef PAPI
#include "papi.h"
#endif

//AML header
#ifdef AML
#include<aml.h>
#include<aml/higher/replicaset.h>
#include<aml/higher/replicaset/hwloc.h>
#endif

// Grid types
#define UNIONIZED 0
#define NUCLIDE 1
#define HASH 2

// Simulation types
#define HISTORY_BASED 1
#define EVENT_BASED 2

// Binary Mode Type
#define NONE 0
#define READ 1
#define WRITE 2

// Starting Seed
#define STARTING_SEED 1070


// Structures
typedef struct{
	double energy;
	double total_xs;
	double elastic_xs;
	double absorbtion_xs;
	double fission_xs;
	double nu_fission_xs;
} NuclideGridPoint;

typedef struct{
	int nthreads;
	long n_isotopes;
	long n_gridpoints;
	int lookups;
	char * HM;
	int grid_type; // 0: Unionized Grid (default)    1: Nuclide Grid
	int hash_bins;
	int particles;
	int simulation_method;
	int binary_mode;
	int kernel_id;
	//Modification by osm
	int port;
} Inputs;

typedef struct{
	int * num_nucs;                     // Length = length_num_nucs;
	double * concs;                     // Length = length_concs
	int * mats;                         // Length = length_mats
	double * unionized_energy_array;    // Length = length_unionized_energy_array
	int * index_grid;                   // Length = length_index_grid
	NuclideGridPoint * nuclide_grid;    // Length = length_nuclide_grid
#ifdef AML
	struct aml_replicaset * num_nucs_replica;
	struct aml_replicaset * concs_replica;
	struct aml_replicaset * unionized_energy_array_replica;
	struct aml_replicaset * index_grid_replica;
	struct aml_replicaset * nuclide_grid_replica;
#endif
	int length_num_nucs;
	int length_concs;
	int length_mats;
	int length_unionized_energy_array;
	long length_index_grid;
	int length_nuclide_grid;
	int max_num_nucs;
	double * p_energy_samples;
	int length_p_energy_samples;
	int * mat_samples;
	int length_mat_samples;
} SimulationData;

// io.c
void logo(int version);
void center_print(const char *s, int width);
void border_print(void);
void fancy_int(long a);
Inputs read_CLI( int argc, char * argv[] );
void print_CLI_error(void);
void print_inputs(Inputs in, int nprocs, int version);
int print_results( Inputs in, int mype, double runtime, int nprocs, unsigned long long vhash );
void binary_write( Inputs in, SimulationData SD );
SimulationData binary_read( Inputs in );

// Simulation.c
unsigned long long run_event_based_simulation(Inputs in, SimulationData SD, int mype);
unsigned long long run_history_based_simulation(Inputs in, SimulationData SD, int mype);
void calculate_micro_xs(   double p_energy, int nuc, long n_isotopes,
                           long n_gridpoints,
                           double * restrict egrid, int * restrict index_data,
                           NuclideGridPoint * restrict nuclide_grids,
                           long idx, double * restrict xs_vector, int grid_type, int hash_bins );
void calculate_macro_xs( double p_energy, int mat, long n_isotopes,
                         long n_gridpoints, int * restrict num_nucs,
                         double * restrict concs,
                         double * restrict egrid, int * restrict index_data,
                         NuclideGridPoint * restrict nuclide_grids,
                         int * restrict mats,
                         double * restrict macro_xs_vector, int grid_type, int hash_bins, int max_num_nucs );
long grid_search( long n, double quarry, double * restrict A);
long grid_search_nuclide( long n, double quarry, NuclideGridPoint * A, long low, long high);
int pick_mat( uint64_t * seed );
double LCG_random_double(uint64_t * seed);
uint64_t fast_forward_LCG(uint64_t seed, uint64_t n);
unsigned long long run_event_based_simulation_optimization_1(Inputs in, SimulationData SD, int mype);

// GridInit.c
SimulationData grid_init_do_not_profile( Inputs in, int mype );

// XSutils.c
int NGP_compare( const void * a, const void * b );
int double_compare(const void * a, const void * b);
size_t estimate_mem_usage( Inputs in );
double get_time(void);

// Materials.c
int * load_num_nucs(long n_isotopes);
int * load_mats( int * num_nucs, long n_isotopes, int * max_num_nucs );
double * load_concs( int * num_nucs, int max_num_nucs );
#endif
