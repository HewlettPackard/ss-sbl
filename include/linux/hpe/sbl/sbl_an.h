/* SPDX-License-Identifier: GPL-2.0 */

/* Copyright 2019-2020 Hewlett Packard Enterprise Development LP */

// This header file includes definitions relating to the autonegotiation
// feature. This information comes largely from Clause 73 of 802.3-2015
//

#ifndef SBL_AN_H
#define SBL_AN_H

// See section 73.6 of the IEEE 802.3-2015 standard for an explanation of the link
// codeword encoding

//
//
// Control word (base page) structure
//
//

// Selector Field
// Bits D0:D4/S0:S4
#define AN_CW_S_BASE_BIT            0
#define AN_CW_S_MASK                0x1F
#define AN_CW_S_802_3               1

// Echoed Nonce
// Bits D5:D9/E0:E4
#define AN_CW_E_BASE_BIT            5
#define AN_CW_E_MASK                (0x1FL << AN_CW_E_BASE_BIT)

// Pause Ability
// Bits D10:D12/C0:C2
#define AN_CW_C_BASE_BIT            10
#define AN_CW_C_MASK                (0x7L << AN_CW_C_BASE_BIT)
#define AN_CW_C_SYMMETRIC_BIT       0
#define AN_CW_C_SYMMETRIC           (1L << AN_CW_C_SYMMETRIC_BIT)
#define AN_CW_C_ASYMMETRIC_BIT      1
#define AN_CW_C_ASYMMETRIC          (1L << AN_CW_C_ASYMMETRIC_BIT)
// C2 is reserved

// Remote Fault
// Bit D13
#define AN_CW_RF_BIT                13
#define AN_CW_RF_MASK               (1L << AN_CW_RF_BIT)

// Ack
// Bit D14
#define AN_CW_ACK_BIT               14
#define AN_CW_ACK_MASK              (1L << AN_CW_ACK_BIT)

// Next Page
// Bit D15
#define AN_CW_NP_BIT                15
#define AN_CW_NP_MASK               (1L << AN_CW_NP_BIT)

// Transmitted Nonce
// Bits D16:D20/T0:T4
#define AN_CW_T_BASE_BIT            16
#define AN_CW_T_MASK                (0x1FL << AN_CW_T_BASE_BIT)

// Technology Ability
// Bits D21:D43/A0:A22
// NB top 2 bits moved into Fec ability field in updated spec ????
#define AN_CW_A_BASE_BIT            21
#define AN_CW_A_MASK                (0x1FFFFFL << AN_CW_A_BASE_BIT)
#define AN_CW_A_1000BASE_KX_BIT     0
#define AN_CW_A_1000BASE_KX         (1L << AN_CW_A_1000BASE_KX_BIT)
#define AN_CW_A_10GBASE_KX4_BIT     1
#define AN_CW_A_10GBASE_KX4         (1L << AN_CW_A_10GBASE_KX4_BIT)
#define AN_CW_A_10GBASE_KR_BIT      2
#define AN_CW_A_10GBASE_KR          (1L << AN_CW_A_10GBASE_KR_BIT)
#define AN_CW_A_40GBASE_KR4_BIT     3
#define AN_CW_A_40GBASE_KR4         (1L << AN_CW_A_40GBASE_KR4_BIT)
#define AN_CW_A_40GBASE_CR4_BIT     4
#define AN_CW_A_40GBASE_CR4         (1L << AN_CW_A_40GBASE_CR4_BIT)
#define AN_CW_A_100GBASE_CR10_BIT   5
#define AN_CW_A_100GBASE_CR10       (1L << AN_CW_A_100GBASE_CR10_BIT)
#define AN_CW_A_100GBASE_KP4_BIT    6
#define AN_CW_A_100GBASE_KP4        (1L << AN_CW_A_100GBASE_KP4_BIT)
#define AN_CW_A_100GBASE_KR4_BIT    7
#define AN_CW_A_100GBASE_KR4        (1L << AN_CW_A_100GBASE_KR4_BIT)
#define AN_CW_A_100GBASE_CR4_BIT    8
#define AN_CW_A_100GBASE_CR4        (1L << AN_CW_A_100GBASE_CR4_BIT)
#define AN_CW_A_25GBASE_KR_S_BIT    9
#define AN_CW_A_25GBASE_KR_S        (1L << AN_CW_A_25GBASE_KR_S_BIT)
#define AN_CW_A_25GBASE_KR_BIT      10
#define AN_CW_A_25GBASE_KR          (1L << AN_CW_A_25GBASE_KR_BIT)
#define AN_CW_A_2P5GBASE_KX_BIT     11
#define AN_CW_A_2P5GBASE_KX         (1L << AN_CW_A_2P5GBASE_KX_BIT)
#define AN_CW_A_5GBASE_KR_BIT       12
#define AN_CW_A_5GBASE_KR           (1L << AN_CW_A_5GBASE_KR_BIT)
#define AN_CW_A_50GBASE_KR_BIT      13
#define AN_CW_A_50GBASE_KR          (1L << AN_CW_A_50GBASE_KR_BIT)
#define AN_CW_A_50GBASE_CR_BIT      AN_CW_A_50GBASE_KR_BIT
#define AN_CW_A_50GBASE_CR          AN_CW_A_50GBASE_KR
#define AN_CW_A_100GBASE_KR2_BIT    14
#define AN_CW_A_100GBASE_KR2        (1L << AN_CW_A_100GBASE_KR2_BIT)
#define AN_CW_A_100GBASE_CR2_BIT    AN_CW_A_100GBASE_KR2_BIT
#define AN_CW_A_100GBASE_CR2        AN_CW_A_100GBASE_KR2
#define AN_CW_A_200GBASE_KR4_BIT    15
#define AN_CW_A_200GBASE_KR4        (1ULL << AN_CW_A_200GBASE_KR4_BIT)
#define AN_CW_A_200GBASE_CR4_BIT    AN_CW_A_200GBASE_KR4_BIT
#define AN_CW_A_200GBASE_CR4        AN_CW_A_200GBASE_KR4
// Bits 16-22 are reserved for future use

// FEC Capability
// Bits D46:D47/F0:F1, D44:D45/F2:F3
#define AN_CW_F_BASE_BIT            44
#define AN_CW_F_MASK                (0xFL << AN_CW_F_BASE_BIT)
#define AN_CW_F_10G_ABILITY_BIT     2
#define AN_CW_F_10G_ABILITY         (1L << AN_CW_F_10G_ABILITY_BIT)
#define AN_CW_F_10G_REQUESTED_BIT   3
#define AN_CW_F_10G_REQUESTED       (1L << AN_CW_F_10G_REQUESTED_BIT)
#define AN_CW_F_25G_RS_REQ_BIT      0
#define AN_CW_F_25G_RS_REQ          (1L << AN_CW_F_25G_RS_REQ_BIT)
#define AN_CW_F_25G_BASER_REQ_BIT   1
#define AN_CW_F_25G_BASER_REQ       (1L << AN_CW_F_25G_BASER_REQ_BIT)


//
//
// Next page structure
//
//

// Next page (same as CW)
// Bit D15
#define AN_NP_NP_MASK               AN_CW_NP_MASK

// Toggle
// Bit D11
#define AN_NP_T_BIT                 11
#define AN_NP_T_MASK                (1L << AN_NP_T_BIT)

// Message page
// Bit D13
#define AN_NP_MP_BIT                13
#define AN_NP_MP_MASK               (1 << AN_NP_MP_BIT)

// Message code
// Bits D0:D10,M0:M10
#define AN_NP_MSG_BASE_BIT          0x0L
#define AN_NP_MSG_MASK              (0x7ffULL << AN_NP_MSG_BASE_BIT)

// Unformatted code field
// Bits D16:D47,U0:U31
#define AN_NP_UCF_BASE_BIT          16
#define AN_NP_UCF_MASK              (0xffffffffULL << AN_NP_UCF_BASE_BIT)

// OUI field
// Bits D16:D39,U0:U23
#define AN_NP_OUI_BASE_BIT          16
#define AN_NP_OUI_MASK              (0xffffffULL << AN_NP_OUI_BASE_BIT)

// user defined data field
// Bits D40:D47,U24:U31
#define AN_NP_USR_BASE_BIT          40
#define AN_NP_USR_MASK              (0xffULL << AN_NP_USR_BASE_BIT)

// Message codes
#define AN_NP_CODE_NULL_MSG         0x1ULL
#define AN_NP_CODE_OUI_MSG          0x5ULL   /* Organizationally Unique Identifier Tagged Message            */
#define AN_NP_CODE_OUI_EXTENDED_MSG 0xBULL   /* Organizationally Unique Identifier Tagged Message (Extended) */

// HPE OUI
#define AN_NP_OUI_HPE               (0xEC9B8BULL << AN_NP_OUI_BASE_BIT)
#define AN_NP_OUI_VER_0_1           (0x01ULL << AN_NP_UCF_BASE_BIT)
#define AN_NP_OUI_VER_MASK          (0xFFULL << AN_NP_UCF_BASE_BIT)

// AN options
#define AN_OPT_LLR                  (1ULL << 0)
#define AN_OPT_ETHER_LLR            (1ULL << 1)
#define AN_OPT_HPC_WITH_LLR         (1ULL << 2)
// TODO: add IPV4 option
#define AN_OPT_BASE_BIT             (AN_NP_UCF_BASE_BIT + 8)
#define AN_OPT_MASK                 (AN_OPT_ETHER_LLR | AN_OPT_HPC_WITH_LLR | AN_OPT_LLR)

// Cassini
#define AN_LP_SUBTYPE_BASE_BIT      (AN_OPT_BASE_BIT + 8)
#define AN_LP_SUBTYPE_MASK          (0x7ULL)

#endif // SBL_AN_H
