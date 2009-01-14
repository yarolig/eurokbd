//#include <windows.h>
#include <sip.h>
#include "ids.h"
void main()
{
    CLSID clsid = CLSID_EuroKbd;
    SipSetCurrentIM(&clsid);
    SipGetCurrentIM(&clsid);
//  SipShowIM(SIPF_ON);
}