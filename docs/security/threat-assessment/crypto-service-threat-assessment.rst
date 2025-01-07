PSA Crypto SP
=============


Scope of evaluation
-------------------

This assessment makes the following assumptions:

    - The PSA Crypto service is deployed to a dedicated S-EL0 SP, and its execution context is physically isolated.
    - The SP has exclusive access to a TRNG device and to the crypto accelerator if present the system.
    - The SP has exclusive access to the HUK (Hardware Unique Key) if present in the system. This is represented as a
      PSA key with built-in key type.
    - The SP relies on the PSA ITS SP for storing persistent data.
    - The SP uses the Logging SP, or the FF-A logging API provided by the SPMC.

Assessment Results
------------------

This section specializes the threats identified in the Generic Threat Model for the crypto service where applicable.
Threats not listed here are mitigated as described in the :doc:`/security/threat-models/generic-threat-model`.

    - :ref:`GEN05 <generic_threat_5>` "External devices connected to the system storing sensitive data."

      Both hardware devices used by the Crypto SP are assumed to be internal, and thus this threat is considered out of
      scope.

    - :ref:`GEN06 <generic_threat_6>` "State of external devices connected to the system might be modified by an
      attacker."

      Both hardware devices used by the Crypto SP are assumed to be internal, and thus this threat is considered out of
      scope.

    - :ref:`GEN07 <generic_threat_7>` "Invalid or conflicting access to shared hardware."

      This threat is considered out of scope due to the defined scope of evaluation.

    - :ref:`GEN08 <generic_threat_8>` "Unauthenticated access to hardware."

      This threat is considered out of scope as shared use of hardware is excluded by the scope of evaluation.

    - :ref:`GEN09 <generic_threat_9>` "Unauthenticated access to sensitive data."

      The PSA Crypto SP isolates its clients at the FF-A level and each FF-A endpoint is restricted to its own asset
      namespace.

      The Crypto SP requires PSA ITS to enforce client isolation and prevent access to its assets from any other
      FF-A endpoint.

    - :ref:`GEN10 <generic_threat_10>` "Time-of-Check to Time-of-Use (TOCTTOU) attack through shared memory."

      The PSA Crypto service provider must ensure data is copied to a secure memory buffer before calling the crypto
      backend to execute sensitive operations.

--------------

*Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.*

SPDX-License-Identifier: BSD-3-Clause
