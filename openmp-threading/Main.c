#include "XSbench_header.h"

/*
#ifdef MPI
#include<mpi.h>
#endif
*/

int main( int argc, char* argv[] )
{
	// =====================================================================
	// Initialization & Command Line Read-In
	// =====================================================================
	int version = 20;
	int mype = 0;
	int nprocs = 1;
	int is_invalid_result = 1;
	double omp_start, omp_end;
	int socket_fd, new_socket;
	int port = 24;
	struct sockaddr_in address;
	int addrlen = sizeof(address);
	int stop;
	char buffer[1024] = {0}; 
	unsigned long long verification;

	#ifdef MPI
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &mype);
	#endif

	#ifdef AML
	aml_init(&argc, &argv);
	#endif

	setvbuf(stdout, NULL, _IOLBF, 0);

	// Process CLI Fields -- store in "Inputs" structure
	Inputs in = read_CLI( argc, argv );

	// Set number of OpenMP Threads
	#ifdef OPENMP
	omp_set_num_threads(in.nthreads); 
	#endif

	// Print-out of Input Summary
	if( mype == 0 )
		print_inputs( in, nprocs, version );

	// =====================================================================
	// Prepare Nuclide Energy Grids, Unionized Energy Grid, & Material Data
	// This is not reflective of a real Monte Carlo simulation workload,
	// therefore, do not profile this region!
	// =====================================================================
	
	SimulationData SD;

	// If read from file mode is selected, skip initialization and load
	// all simulation data structures from file instead
	if( in.binary_mode == READ )
		SD = binary_read(in);
	else
		SD = grid_init_do_not_profile( in, mype );

	// If writing from file mode is selected, write all simulation data
	// structures to file
	if( in.binary_mode == WRITE && mype == 0 )
		binary_write(in, SD);


	// =====================================================================
	// Cross Section (XS) Parallel Lookup Simulation
	// This is the section that should be profiled, as it reflects a 
	// realistic continuous energy Monte Carlo macroscopic cross section
	// lookup kernel.
	// =====================================================================

	// get port
	port = in.port;	

	// Create Socket
	if((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		perror("Socket create error\n");
		return -1;
	}

	address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);

	// Bind Socket
	if(bind(socket_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
		perror("Socket bind error\n");
		return -1;
	}
	
	// Listen Socket	
	if(listen(socket_fd, 3) < 0) {
		perror("Socket listen error\n");
		return -1;
	}

	
	if( mype == 0 )
	{
		printf("\n");
		border_print();
		center_print("SIMULATION", 79);
		border_print();
	}
	
	
	printf("BUILD END\n");
	while(1) {
		// Accept Client
		if((new_socket = accept(socket_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
			perror("Socket accept error\n");
			return -1;
		}

		// Get data
		read(new_socket, buffer, 1024);
		stop = atoi(buffer);

		if(stop < 0)
			return -1;

		// Start Simulation Timer
		omp_start = get_time();

		// Run simulation
		if( in.simulation_method == EVENT_BASED )
		{
			if( in.kernel_id == 0 )
				verification = run_event_based_simulation(in, SD, mype);
			else if( in.kernel_id == 1 )
				verification = run_event_based_simulation_optimization_1(in, SD, mype);
			else
			{
				printf("Error: No kernel ID %d found!\n", in.kernel_id);
				exit(1);
			}
		}
		else
			verification = run_history_based_simulation(in, SD, mype);

		if( mype == 0)	
		{	
			printf("\n" );
			printf("Simulation complete.\n" );
		}

		// End Simulation Timer
		omp_end = get_time();

		// =====================================================================
		// Output Results & Finalize
		// =====================================================================

		// Final Hash Step
		verification = verification % 999983;

		// Print / Save Results and Exit
		is_invalid_result = print_results( in, mype, omp_end-omp_start, nprocs, verification );
		
		send(new_socket, buffer, strlen(buffer), 0); 
		if(stop == 0)
			continue;
		if(stop == 1)
			break;
	}

	#ifdef MPI
	MPI_Finalize();
	#endif

	#ifdef AML
	aml_finalize();
	#endif

	return is_invalid_result;
}
