PSA Attestation SP
==================

Scope of evaluation
-------------------

This assessment makes the following assumptions:

    - The PSA Attestation service is deployed to a dedicated S-EL0 SP, and its execution context is physically isolated.
    - The solution uses the Logging SP, or the FF-A logging API provided by the SPMC.
    - The Attestation SP uses a PSA Crypto SP for executing cryptographic operations.
    - The SPMC passes a copy of the measurements made by earlier boot stages (BL1 and BL2) to the Attestation SP as boot
      arguments. The system must protect the measurements from tampering before passing to the SP.

Assessment Results
------------------

This section specializes the threats identified in the Generic Threat Model for the attestation service where applicable.
Threats not listed here are mitigated as described in the :doc:`/security/threat-models/generic-threat-model`.

    - :ref:`GEN05 <generic_threat_5>` "External devices connected to the system storing sensitive data."

      The Attestation SP does not require any external devices for its operation, even when the built-in PSA Crypto is
      used. Therefore, this threat is considered out of scope.

    - :ref:`GEN06 <generic_threat_6>` "State of external devices connected to the system might be modified by an
      attacker."

     The Attestation SP does not require any external devices for its operation, even when the built-in PSA Crypto is
     used. Therefore, this threat is considered out of scope.

    - :ref:`GEN07 <generic_threat_7>` "Invalid or conflicting access to shared hardware."

      The Attestation SP does not require any external devices for its operation, even when the built-in PSA Crypto is
      used. Therefore, this threat is considered out of scope.

    - :ref:`GEN08 <generic_threat_8>` "Unauthenticated access to hardware."

      The Attestation SP does not require any external devices for its operation, even when the built-in PSA Crypto is
      used. Therefore, this threat is considered out of scope.

    - :ref:`GEN09 <generic_threat_9>` "Unauthenticated access to sensitive data."

      The PSA Attestation SP is designed to present a uniform view to all clients, so the SP does not need to enforce
      client isolation itself.

      The Attestation SP requires the PSA Crypto SPs to enforce client isolation and prevent other FF-A endpoints
      from accessing its assets.

    - :ref:`GEN10 <generic_threat_10>` "Time-of-Check to Time-of-Use (TOCTTOU) attack through shared memory."

      The Attestation service provider must ensure data is copied to a secure memory buffer before calling the PSA Crypto
      implementation to execute sensitive operations.

--------------

*Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.*

SPDX-License-Identifier: BSD-3-Clause
