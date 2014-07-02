Cloud_AVailability
==================

This file use the Inter Processor Interrupt (IPI) to perform the availability in the cloud

v3: use Yinqian's code.

The attacker has 2 cores, one is for sending IPI, another is for receiving IPI, and co-residing with victim

For pinned cpu, it works.

For float cpu, it does not work, as it is hard to co-reside with the victim.



