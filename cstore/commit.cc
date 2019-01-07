/*
 * file:	commit.cc
 * purpose:	Implementation of the Commit class
 *
 * revision history:
 *   Jiantao Huang		    initial version
 */

#include "commit.h"

/*
 * Commit base class
 */

Commit::Commit()
{
}

Commit::~Commit() throw ()
{
}

Commit::Commit(const Commit& obj)
{
}

Commit& Commit::operator=(const Commit& obj)
{
	return *this;
}

/*
 * Synchronous Commit class
 */

SynchCommit::SynchCommit() 
{
}

SynchCommit::~SynchCommit() throw()
{
}

SynchCommit::SynchCommit(const SynchCommit& obj)
		: Commit (obj)
{
}

SynchCommit& SynchCommit::operator=(const SynchCommit& obj)
{
	Commit::operator=(obj);
	return *this;
}

void SynchCommit::commit(vector<COMMIT_ARGS>& commitArgs,
						list<pthread_t>& tids)
{	
	pthread_t      a_tid;

    tids.clear();

    /*
     * Now, do Synch Storage Commitment
     */
    for(unsigned int i=0; i < commitArgs.size(); i++)
	{
		::Message(MNOTE, toEndUser | toService | MLoverall, 
				"Synchronous storage commitment from %s for data stored on %s.",
				commitArgs[i].options.RemoteHostname, commitArgs[i].result->storageHostName.in());

// Use EitherStorageCommitment in place of SynchStorageCommitment will not slow down the Synch commit
// but will provide extra protection against incorrect configuration and increase the reliablity of cstore
//		if( pthread_create(&a_tid, NULL, SynchStorageCommitment, (void*)commit_args) != 0)
		if( pthread_create(&a_tid, NULL, EitherStorageCommitment, (void*)&commitArgs[i]) != 0)
		{
			::Message( MALARM, toEndUser | toService | MLoverall, 
					"#Software error: cannot create a thread for synch storage commitment from %s for data stored on %s.",
					commitArgs[i].options.RemoteHostname, commitArgs[i].result->storageHostName.in());
			tids.push_back((pthread_t)(-1));
		}

		tids.push_back(a_tid);
	}
}

/*
 * Asynchronous Commit class
 */

AsynchCommit::AsynchCommit() 
{
}

AsynchCommit::~AsynchCommit() throw()
{
}

AsynchCommit::AsynchCommit(const AsynchCommit& obj)
		: Commit (obj)
{
}

AsynchCommit& AsynchCommit::operator=(const AsynchCommit& obj)
{
	Commit::operator=(obj);
	return *this;
}

void AsynchCommit::commit(vector<COMMIT_ARGS>& commitArgs,
						list<pthread_t>& tids)
{	
	pthread_t      a_tid;

    tids.clear();

    /*
     * Now, do Asynch Storage Commitment
     */
    for(unsigned int i=0; i < commitArgs.size(); i++)
	{
		::Message(MNOTE, toEndUser | toService | MLoverall, 
				"Asynchronous storage commitment from %s for data stored on %s.",
				commitArgs[i].options.RemoteHostname, commitArgs[i].result->storageHostName.in());

		if( pthread_create(&a_tid, NULL, AsynchStorageCommitment, (void*)&commitArgs[i]) != 0)
		{
			::Message( MALARM, toEndUser | toService | MLoverall, 
					"#Software error: cannot create a thread for asynch storage commitment from %s for data stored on %s.",
					commitArgs[i].options.RemoteHostname, commitArgs[i].result->storageHostName.in());
			tids.push_back((pthread_t)(-1));
		}

		tids.push_back(a_tid);
	}
}

/*
 * Either Commit class
 */

EitherCommit::EitherCommit() 
{
}

EitherCommit::~EitherCommit() throw()
{
}

EitherCommit::EitherCommit(const EitherCommit& obj)
		: Commit (obj)
{
}

EitherCommit& EitherCommit::operator=(const EitherCommit& obj)
{
	Commit::operator=(obj);
	return *this;
}

void EitherCommit::commit(vector<COMMIT_ARGS>& commitArgs,
						list<pthread_t>& tids)
{	
	pthread_t      a_tid;

    tids.clear();

    /*
     * Now, do Either Storage Commitment
     */
    for(unsigned int i=0; i < commitArgs.size(); i++)
	{
		::Message(MNOTE, toEndUser | toService | MLoverall, 
				  "'Either storage commitment' from %s for data stored on %s.",
				  commitArgs[i].options.RemoteHostname, commitArgs[i].result->storageHostName.in());

		if( pthread_create(&a_tid, NULL, EitherStorageCommitment, (void*)&commitArgs[i]) != 0)
		{
			::Message( MALARM, toEndUser | toService | MLoverall, 
					"#Software error: cannot create a thread for 'either storage commitment' from %s for data stored on %s.",
					commitArgs[i].options.RemoteHostname, commitArgs[i].result->storageHostName.in());
			tids.push_back((pthread_t)(-1));
		}

		tids.push_back(a_tid);
	}
}
