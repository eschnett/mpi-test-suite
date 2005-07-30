/*
 * File: tst_p2p_alltoall.c
 *
 * Functionality:
 *  This test features sending from every processes (the same array)
 *  to every one else, sending non-blocking and then doing a waitall.
 *  Works with intra- and inter- communicators and up to now with
 *  any C (standard and struct) type.
 *
 * Author: Rainer Keller
 *
 * Date: Oct 14th 2003
 */
#include "mpi.h"
#include "mpi_test_suite.h"

#undef DEBUG
#define DEBUG(x)

static char * send_buffer = NULL;
static char ** recv_buffer = NULL;
static MPI_Request * req_buffer = NULL;
static MPI_Status * status_buffer = NULL;

int tst_p2p_alltoall_init (const struct tst_env * env)
{
  MPI_Comm comm;
  int comm_rank;
  int comm_size;
  int i;

  DEBUG (printf ("(Rank:%d) env->comm:%d env->type:%d env->values_num:%d\n",
                 tst_global_rank, env->comm, env->type, env->values_num));

  /*
   * Now, initialize the buffer
   */
  comm = tst_comm_getcomm (env->comm);
  MPI_CHECK (MPI_Comm_rank (comm, &comm_rank));
  
  if (tst_comm_getcommclass (env->comm) == TST_MPI_INTER_COMM)
    MPI_CHECK (MPI_Comm_remote_size (comm, &comm_size));
  else
    MPI_CHECK (MPI_Comm_size (comm, &comm_size));

  send_buffer = tst_type_allocvalues (env->type, env->values_num);
  tst_type_setstandardarray (env->type, env->values_num,
			     send_buffer, comm_rank);

  if ((req_buffer = malloc (2 * sizeof (MPI_Request) * comm_size)) == NULL)
    ERROR (errno, "malloc");

  if ((status_buffer = malloc (2 * sizeof (MPI_Status) * comm_size)) == NULL)
    ERROR (errno, "malloc");

  if ((recv_buffer = malloc (sizeof (char *) * comm_size)) == NULL)
    ERROR (errno, "malloc");
  
  for (i=0; i < comm_size; i++)
    {
      recv_buffer[i] = tst_type_allocvalues (env->type, env->values_num);
    }

  return 0;
}

int tst_p2p_alltoall_run (const struct tst_env * env)
{
  int comm_size;
  int comm_rank;
  int rank;
  MPI_Comm comm;
  MPI_Datatype type;

  comm = tst_comm_getcomm (env->comm);
  type = tst_type_getdatatype (env->type);

  if (tst_comm_getcommclass (env->comm) == TST_MPI_INTRA_COMM)
    MPI_CHECK (MPI_Comm_size (comm, &comm_size));
  else
    MPI_CHECK (MPI_Comm_remote_size (comm, &comm_size));

  MPI_CHECK (MPI_Comm_rank (comm, &comm_rank));

  DEBUG (printf ("(Rank:%d) comm_size:%d comm_rank:%d\n",
                 tst_global_rank, comm_size, comm_rank));

  /*
   * We send bottom-up, but receive top-down
   */
  for (rank = 0; rank < comm_size; rank++)
    {
      req_buffer[2*rank + 0] = MPI_REQUEST_NULL;
      MPI_CHECK (MPI_Isend (send_buffer, env->values_num, type,
			    rank, 4711, comm,
			    &(req_buffer[2*rank + 0])));

      if (req_buffer[2*rank + 0] == MPI_REQUEST_NULL)
	ERROR (EINVAL, "Error in request after MPI_Isend");

      req_buffer[2*rank + 1] = MPI_REQUEST_NULL;
      MPI_CHECK (MPI_Irecv (recv_buffer[rank], env->values_num, type,
			    comm_size - rank - 1, 4711, comm,
			    &(req_buffer[2*rank + 1])));

      if (req_buffer[2*rank + 1] == MPI_REQUEST_NULL)
	ERROR (EINVAL, "Error in request after MPI_Irecv");
    }

  
  MPI_CHECK (MPI_Waitall (comm_size*2, req_buffer, status_buffer));

  for (rank = 0; rank < comm_size; rank++)
    {
      if (req_buffer[2*rank + 0] != MPI_REQUEST_NULL ||
          req_buffer[2*rank + 1] != MPI_REQUEST_NULL)
        ERROR (EINVAL, "Requests are not reset to MPI_REQUEST_NULL");

      if (status_buffer[2*rank+1].MPI_SOURCE != comm_size - rank - 1  ||
	  status_buffer[2*rank+1].MPI_TAG != 4711)
	{
	  DEBUG (printf ("(Rank:%d) rank:%d comm_size:%d should be:%d "
			 "MPI_SOURCE:%d MPI_TAG:%d\n",
			 comm_rank, rank, comm_size, comm_size - rank-1,
			 status_buffer[2*rank+1].MPI_SOURCE,
			 status_buffer[2*rank+1].MPI_TAG));
	  ERROR (EINVAL, "Error in communication");
	}
      tst_type_checkstandardarray (env->type, env->values_num,
				   recv_buffer[rank], comm_size - rank - 1);
    }
  return 0;
}

int tst_p2p_alltoall_cleanup (const struct tst_env * env)
{
  MPI_Comm comm;
  int i;
  int comm_size;


  free (send_buffer);

  comm = tst_comm_getcomm (env->comm);

  if (tst_comm_getcommclass (env->comm) == TST_MPI_INTER_COMM)
    MPI_CHECK (MPI_Comm_remote_size (comm, &comm_size));
  else
    MPI_CHECK (MPI_Comm_size (comm, &comm_size));

  for (i = 0; i < comm_size; i++)
    free (recv_buffer[i]);
    
  free (recv_buffer);
  free (req_buffer);
  free (status_buffer);

  return 0;
}
