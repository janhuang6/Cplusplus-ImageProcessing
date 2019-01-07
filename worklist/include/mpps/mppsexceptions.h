#ifndef _MPPS_EXCEPTIONS_H_
#define _MPPS_EXCEPTIONS_H_

/*
 * file:        mppsexceptions.h
 * purpose:     define common error conditions for mpps which might require special action
 *
 * $Name: Atlantis710R01_branch $
 */

#include <string>
#include <acquire/acqexception.h>

// These exceptions may generate core files
class MppsException: public AcqException
{
public:
	explicit MppsException(const std::string &message) throw(): AcqException(message) {};
};

// Should never generate core file, not derived from MppsException
class MppsNoFacilityException: public MppsException
{
public:
	explicit MppsNoFacilityException(const std::string &message) throw(): MppsException(message) { _trapBeforeExit=true; };
};

// Should never generate core file, not derived from MppsException
class MppsNotLicensedException: public MppsException
{
public:
	explicit MppsNotLicensedException(const std::string &message) throw(): MppsException(message) { _trapBeforeExit=true; };
};

// Should never generate core file, not derived from MppsException
class MppsNotConfiguredException: public MppsException
{
public:
	explicit MppsNotConfiguredException(const std::string &message) throw(): MppsException(message) { _trapBeforeExit=true; };
};


#endif // _MPPS_EXCEPTIONS_H_

