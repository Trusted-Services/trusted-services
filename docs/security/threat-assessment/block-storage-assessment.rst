Block Storage SP
================


Scope of evaluation
-------------------

This assessment makes the following assumptions:

    - The Block Storage service is deployed to a dedicated S-EL0 SP, and its execution context is isolated from other
      services, using |TRUSTZONE|.
    - The SP owns the storage HW and has exclusive access to it. If not, the storage device must implement
      authentication and shall deny any write request not originating from the Block Storage SP.
    - The SP will use the Logging SP, or the FF-A logging API provided by the SPMC.

Assessment Results
------------------

This section specializes the threats identified in the Generic Threat Model for the storage service where needed.
Threats not listed here are mitigated as described in the :doc:`/security/threat-models/generic-threat-model`.

    - :ref:`GEN05 <generic_threat_5>` "External devices connected to the system storing sensitive data."

      This threat is in scope if the platform uses an external device as the non-volatile storage for the service. The
      service must protect data in transit at DF9 and DF11.

    - :ref:`GEN06 <generic_threat_6>` "State of external devices connected to the system might be modified by an attacker."

      This threat is in scope when the platform relies on an external device for non-volatile storage used by the
      service. To mitigate the risk, the service must implement platform-specific measures to protect the data.

    - :ref:`GEN07 <generic_threat_7>` "Invalid or conflicting access to shared hardware."

      This threat is out of scope due to the scope of evaluation.

    - :ref:`GEN08 <generic_threat_8>` "Unauthenticated access to hardware."

      This threat is out of scope as shared usage of hardware is excluded by the scope of evaluation.

    - :ref:`GEN09 <generic_threat_9>` "Unauthenticated access to sensitive data."

      The Block Storage SP isolates clients at FF-A level and each FF-A endpoint is isolated to its own namespace.

      If the selected storage backed relies on another service like the Block Storage SP, the backend service must
      implement access control and client isolation too.

    - :ref:`GEN10 <generic_threat_10>` "Time-of-Check to Time-of-Use (TOCTTOU) attack through shared memory."

      The Block Storage service will not read the input data multiple times and thus this threat is mitigated.


--------------

*Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.*

SPDX-License-Identifier: BSD-3-Clause
