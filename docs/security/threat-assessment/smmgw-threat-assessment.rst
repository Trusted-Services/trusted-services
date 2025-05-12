SmmGW SP
========


Scope of evaluation
-------------------

This assessment makes the following assumptions:

    - The SmmGW service is deployed to a dedicated S-EL0 SP, and its execution context is physically isolated.
    - The SP uses the PSA PS SP for storing persistent data.
    - The solution uses the Logging SP, or the FF-A logging API provided by the SPMC.
    - The SmmGW uses a PSA Crypto implementation for executing cryptographic operations.

        - This implementation can be built-in and running within the isolation boundary of the SP.
        - The PSA Crypto implementation can be external and may run in another SP.

Assessment Results
------------------

This section specializes the threats identified in the Generic Threat Model for the SmmGW service where applicable.
Threats not listed here are mitigated as described in the :doc:`/security/threat-models/generic-threat-model`.

    - :ref:`GEN05 <generic_threat_5>` "External devices connected to the system storing sensitive data."

      The SmmGW SP does not require any external devices for its operation, even when the built-in PSA Crypto is used.
      Therefore, this threat is considered out of scope.

    - :ref:`GEN06 <generic_threat_6>` "State of external devices connected to the system might be modified by an
      attacker."

      The SmmGW SP does not require any external devices for its operation, even when the built-in PSA Crypto is used.
      Therefore, this threat is considered out of scope.

    - :ref:`GEN07 <generic_threat_7>` "Invalid or conflicting access to shared hardware."

      The SmmGW SP does not require any external devices for its operation, even when the built-in PSA Crypto is used.
      Therefore, this threat is considered out of scope.

    - :ref:`GEN08 <generic_threat_8>` "Unauthenticated access to hardware."

      The SmmGW SP does not require any external devices for its operation, even when the built-in PSA Crypto is used.
      Therefore, this threat is considered out of scope.

    - :ref:`GEN09 <generic_threat_9>` "Unauthenticated access to sensitive data."

      The UEFI Variable Service is designed to present a uniform view to all clients, so SmmGW does not need to enforce
      client isolation itself.

      The SmmGW SP requires PSA PS and the PSA Crypto SPs to enforce client isolation and prevent other FF-A endpoints
      to access its assets.

    - :ref:`GEN10 <generic_threat_10>` "Time-of-Check to Time-of-Use (TOCTTOU) attack through shared memory."

      The SmmGW service provider must ensure data is copied to a secure memory buffer before calling the psa crypto
      implementation to execute sensitive operations.

--------------

*Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.*

SPDX-License-Identifier: BSD-3-Clause
