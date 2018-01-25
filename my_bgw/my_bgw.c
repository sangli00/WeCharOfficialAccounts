#include "postgres.h"
#include "fmgr.h"
#include "postmaster/bgworker.h"
#include <unistd.h>
#include "storage/proc.h"
#include "utils/fmgrprotos.h"
#include "miscadmin.h"
#include "storage/ipc.h"
#include "pgstat.h"
#include "utils/timestamp.h"
#include "utils/builtins.h"
#include "utils/memutils.h"
#include "inttypes.h"
#include "access/xact.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

void _PG_init(void);
void my_bgw_Main(Datum args);
static void run_my_bgw();


static volatile sig_atomic_t got_sighup = false;
static volatile sig_atomic_t got_sigterm = false;

static void my_bgw_sighup(SIGNAL_ARGS);
static void my_bgw_sigterm(SIGNAL_ARGS);

static MemoryContext my_bgw_ctx = NULL;

static void
my_bgw_sighup(SIGNAL_ARGS)
{

	int save_errno = errno;
	got_sighup = true;
	if (MyProc != NULL)
	{
		SetLatch(&MyProc->procLatch);
	}
	errno = save_errno;
}

static void
my_bgw_sigterm(SIGNAL_ARGS)
{
	int save_errno = errno;
	got_sigterm = true;

	if (MyProc != NULL)
	{
		SetLatch(&MyProc->procLatch);
	}
	errno = save_errno;
}

static void
my_bgw_exit(int code)
{
	if(my_bgw_ctx) {
                MemoryContextDelete(my_bgw_ctx);
                my_bgw_ctx  = NULL;
        }
	proc_exit(code);
}

void
my_bgw_Main(Datum args)
{

	pqsignal(SIGHUP, my_bgw_sighup);
	pqsignal(SIGTERM, my_bgw_sigterm);

	BackgroundWorkerUnblockSignals();

	my_bgw_ctx = AllocSetContextCreate(CurrentMemoryContext,
                                            "my_bgw_ctx",
                                            ALLOCSET_DEFAULT_MINSIZE,
                                            ALLOCSET_DEFAULT_INITSIZE,
                                            ALLOCSET_DEFAULT_MAXSIZE);
	

	MemoryContextSwitchTo(my_bgw_ctx);

	while(!got_sigterm)
	{
		char *result_text;			
		Datum ts;
		Datum result;
		int rc;
		TimestampTz currentTime = 0;

		rc = WaitLatch(MyLatch,
                                           WL_LATCH_SET | WL_TIMEOUT | WL_POSTMASTER_DEATH,
                                           0L,
                                           PG_WAIT_EXTENSION);

                ResetLatch(MyLatch);

                if (got_sighup)
		{
			got_sighup = false;
		}

		if (rc & WL_POSTMASTER_DEATH)
			my_bgw_exit(1);		

		CHECK_FOR_INTERRUPTS();	

		currentTime = GetCurrentTimestamp();
		ts = TimestampTzGetDatum(currentTime);
	
		result = DirectFunctionCall1(timestamptz_out,ts);	
		result_text = DatumGetCString(result);
		elog(LOG,"This clock is :%s\n",result_text);
		sleep(5);
		
		MemoryContextReset(my_bgw_ctx);
	}
	my_bgw_exit(0);
}

static void
run_my_bgw()
{
	BackgroundWorker worker;

	worker.bgw_flags = BGWORKER_SHMEM_ACCESS | BGWORKER_BACKEND_DATABASE_CONNECTION;
        worker.bgw_start_time = BgWorkerStart_RecoveryFinished;
        worker.bgw_notify_pid = 0;
        worker.bgw_restart_time = 1; 
        worker.bgw_main_arg = (Datum)NULL;

	sprintf(worker.bgw_library_name, "my_bgw");
	sprintf(worker.bgw_function_name, "my_bgw_Main");
	snprintf(worker.bgw_name, BGW_MAXLEN, "my_bgw_scheduler");

	RegisterBackgroundWorker(&worker);
}


void _PG_init(void){

	run_my_bgw();	
}

