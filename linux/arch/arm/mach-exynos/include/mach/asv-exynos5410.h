/* linux/arch/arm/mach-exynos/include/mach/asv-exynos5410.h
*
* Copyright (c) 2012 Samsung Electronics Co., Ltd.
*              http://www.samsung.com/
*
* EXYNOS5410 - Adoptive Support Voltage Header file
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_EXYNOS5410_ASV_H
#define __ASM_ARCH_EXYNOS5410_ASV_H __FILE__

#define ARM_DVFS_LEVEL_NR		(17)
#define ARM_ASV_GRP_NR			(13)
#define ARM_MAX_VOLT			(1362500)
#define INT_DVFS_LEVEL_NR		(9)
#define INT_ASV_GRP_NR			(13)
#define INT_MAX_VOLT			(1212500)
#define G3D_DVFS_LEVEL_NR		(6)
#define G3D_ASV_GRP_NR			(13)
#define G3D_MAX_VOLT			(1150000)
#define MIF_DVFS_LEVEL_NR		(4)
#define MIF_ASV_GRP_NR			(13)
#define MIF_MAX_VOLT			(1025000)
#define KFC_DVFS_LEVEL_NR		(12)
#define KFC_ASV_GRP_NR			(13)
#define KFC_MAX_VOLT			(1312500)

static unsigned int refer_table_get_asv[2][ARM_ASV_GRP_NR] = {
	{ 0, 9, 13, 17, 22, 29, 37, 47, 58, 72,  87, 100, 999},
	{ 0, 0, 40, 43, 46, 49, 52, 55, 58, 60, 999, 999, 999},
};

static unsigned int refer_use_table_get_asv[2][ARM_ASV_GRP_NR] = {
	{ 0, 1,  1,  1,  1,  1,  1,  1,  1,  1,   1,   1,   1},
	{ 0, 0,  1,  1,  1,  1,  1,  1,  1,  1,   0,   0,   1},
};

static unsigned int arm_asv_abb_info[ARM_ASV_GRP_NR] = {
	ABB_X080, ABB_X080, ABB_X080, ABB_X080, ABB_X080, ABB_BYPASS, ABB_BYPASS, ABB_BYPASS, ABB_BYPASS, ABB_BYPASS, ABB_BYPASS, ABB_X120, ABB_X120
};

static unsigned int int_asv_abb_info[INT_ASV_GRP_NR] = {
	ABB_X120, ABB_X120, ABB_X120, ABB_X120, ABB_X120, ABB_X120, ABB_X120, ABB_X120, ABB_X120, ABB_X120, ABB_X120, ABB_X120, ABB_X120
};

static unsigned int arm_asv_volt_info[ARM_DVFS_LEVEL_NR][ARM_ASV_GRP_NR + 1] = {
	{ 1800000, 1287500, 1275000, 1262500, 1250000, 1237500, 1250000, 1237500, 1225000, 1212500, 1187500, 1162500, 1162500, 1150000},
	{ 1700000, 1237500, 1225000, 1212500, 1200000, 1187500, 1200000, 1187500, 1175000, 1162500, 1137500, 1112500, 1112500, 1100000},
	{ 1600000, 1200000, 1187500, 1175000, 1162500, 1150000, 1162500, 1150000, 1137500, 1125000, 1100000, 1075000, 1075000, 1062500},
	{ 1500000, 1175000, 1162500, 1150000, 1137500, 1125000, 1137500, 1125000, 1112500, 1100000, 1075000, 1050000, 1050000, 1037500},
	{ 1400000, 1137500, 1125000, 1112500, 1100000, 1087500, 1100000, 1087500, 1075000, 1062500, 1037500, 1012500, 1012500, 1000000},
	{ 1300000, 1100000, 1087500, 1075000, 1062500, 1050000, 1062500, 1050000, 1037500, 1025000, 1000000,  975000,  975000,  975000},
	{ 1200000, 1062500, 1050000, 1037500, 1025000, 1012500, 1025000, 1012500, 1000000,  987500,  975000,  962500,  962500,  950000},
	{ 1100000, 1037500, 1025000, 1012500, 1000000,  987500, 1000000,  987500,  975000,  962500,  950000,  937500,  937500,  925000},
	{ 1000000, 1000000,  987500,  975000,  962500,  950000,  962500,  950000,  937500,  937500,  925000,  925000,  925000,  912500},
	{  900000,  962500,  950000,  937500,  925000,  912500,  925000,  912500,  912500,  912500,  912500,  912500,  912500,  912500},
	{  800000,  925000,  912500,  900000,  900000,  900000,  900000,  900000,  900000,  900000,  900000,  900000,  900000,  900000},
	{  700000,  925000,  912500,  900000,  900000,  900000,  900000,  900000,  900000,  900000,  900000,  900000,  900000,  900000},
	{  600000,  925000,  912500,  900000,  900000,  900000,  900000,  900000,  900000,  900000,  900000,  900000,  900000,  900000},
	{  500000,  925000,  912500,  900000,  900000,  900000,  900000,  900000,  900000,  900000,  900000,  900000,  900000,  900000},
	{  400000,  925000,  912500,  900000,  900000,  900000,  900000,  900000,  900000,  900000,  900000,  900000,  900000,  900000},
	{  300000,  925000,  912500,  900000,  900000,  900000,  900000,  900000,  900000,  900000,  900000,  900000,  900000,  900000},
	{  200000,  925000,  912500,  900000,  900000,  900000,  900000,  900000,  900000,  900000,  900000,  900000,  900000,  900000},
};

static unsigned int kfc_asv_volt_info[KFC_DVFS_LEVEL_NR][KFC_ASV_GRP_NR + 1] = {
	{ 1300000, 1312500, 1287500, 1262500, 1250000, 1237500, 1225000, 1212500, 1200000, 1187500, 1175000, 1175000, 1175000, 1175000},
	{ 1200000, 1312500, 1287500, 1262500, 1250000, 1237500, 1225000, 1212500, 1200000, 1187500, 1175000, 1175000, 1175000, 1175000},
	{ 1100000, 1250000, 1225000, 1200000, 1187500, 1175000, 1162500, 1150000, 1137500, 1125000, 1112500, 1112500, 1112500, 1112500},
	{ 1000000, 1187500, 1162500, 1137500, 1125000, 1112500, 1100000, 1087500, 1075000, 1062500, 1050000, 1050000, 1050000, 1050000},
	{  900000, 1125000, 1100000, 1075000, 1062500, 1050000, 1037500, 1025000, 1012500, 1000000,  987500,  987500,  987500,  987500},
	{  800000, 1075000, 1050000, 1025000, 1012500, 1000000,  987500,  975000,  962500,  950000,  950000,  950000,  950000,  950000},
	{  700000, 1025000, 1000000,  975000,  962500,  950000,  950000,  950000,  950000,  950000,  950000,  950000,  950000,  950000},
	{  600000,  975000,  950000,  950000,  950000,  950000,  950000,  950000,  950000,  950000,  950000,  950000,  950000,  950000},
	{  500000,  950000,  950000,  950000,  950000,  950000,  950000,  950000,  950000,  950000,  950000,  950000,  950000,  950000},
	{  400000,  950000,  950000,  950000,  950000,  950000,  950000,  950000,  950000,  950000,  950000,  950000,  950000,  950000},
	{  300000,  950000,  950000,  950000,  950000,  950000,  950000,  950000,  950000,  950000,  950000,  950000,  950000,  950000},
	{  200000,  950000,  950000,  950000,  950000,  950000,  950000,  950000,  950000,  950000,  950000,  950000,  950000,  950000},
};

static unsigned int int_mif_lv0_asv_volt_info[INT_DVFS_LEVEL_NR][INT_ASV_GRP_NR + 1] = {
	{ 800000, 1100000, 1075000, 1050000, 1037500, 1025000, 1012500, 1000000,  987500,  987500,  975000,  975000,  975000,  975000},
	{ 700000, 1075000, 1050000, 1025000, 1012500, 1000000,  987500,  975000,  962500,  962500,  950000,  950000,  950000,  950000},
	{ 400000,	1075000, 1050000,	1025000, 1012500,	1000000,	987500,	 975000,	962500,  962500,	950000,  950000,	950000,	 950000	},
	{ 267000,	1075000, 1050000,	1025000, 1012500,	1000000,	987500,	 975000,	962500,	 962500,	950000,  950000,	950000,	 950000	},
	{ 200000, 1075000, 1050000, 1025000, 1012500, 1000000,  987500,  975000,  962500,  962500,  950000,  950000,  950000,  950000},
	{ 160000, 1075000, 1050000, 1025000, 1012500, 1000000,  987500,  975000,  962500,  962500,  950000,  950000,  950000,  950000},
	{ 100000, 1075000, 1050000, 1025000, 1012500, 1000000,  987500,  975000,  962500,  962500,  950000,  950000,  950000,  950000},
	{  50000, 1075000, 1050000, 1025000, 1012500, 1000000,  987500,  975000,  962500,  962500,  950000,  950000,  950000,  950000}
};

static unsigned int int_mif_lv1_asv_volt_info[INT_DVFS_LEVEL_NR][INT_ASV_GRP_NR + 1] = {
	{ 800000, 1100000, 1075000, 1050000, 1037500, 1025000, 1012500, 1000000,  987500,  987500,  975000,  975000,  975000,  975000},
	{ 700000, 1075000, 1050000, 1025000, 1012500, 1000000,  987500,  975000,  962500,  962500,  950000,  950000,  950000,  950000},
	{ 400000, 1050000, 1025000, 1000000,  987500,  975000,  962500,  950000,  937500,  925000,  912500,  900000,  887500,  875000},
	{ 267000, 1050000, 1025000, 1000000,  987500,  975000,  962500,  950000,  937500,  925000,  912500,  900000,  887500,  875000},
	{ 200000, 1050000, 1025000, 1000000,  987500,  975000,  962500,  950000,  937500,  925000,  912500,  900000,  887500,  875000},
	{ 160000, 1050000, 1025000, 1000000,  987500,  975000,  962500,  950000,  937500,  925000,  912500,  900000,  887500,  875000},
	{ 100000, 1050000, 1025000, 1000000,  987500,  975000,  962500,  950000,  937500,  925000,  912500,  900000,  887500,  875000},
	{  50000, 1000000,  975000,  950000,  937500,  912500,  900000,  887500,  862500,  850000,  850000,  850000,  850000,  850000},

};

static unsigned int int_mif_lv2_asv_volt_info[INT_DVFS_LEVEL_NR][INT_ASV_GRP_NR + 1] = {
	{ 800000, 1100000, 1075000, 1050000, 1037500, 1025000, 1012500, 1000000,  987500,  987500,  975000,  975000,  975000,  975000},
	{ 700000, 1075000, 1050000, 1025000, 1012500, 1000000,  987500,  975000,  962500,  962500,  950000,  950000,  950000,  950000},
	{ 400000, 1012500,  987500,  962500,  950000,  950000,  937500,  925000,  912500,  900000,  887500,  887500,  887500,  887500},
	{ 267000, 1012500,  987500,  962500,  950000,  950000,  937500,  925000,  912500,  900000,  887500,  887500,  887500,  887500},
	{ 200000, 1012500,  987500,  962500,  950000,  950000,  937500,  925000,  912500,  900000,  887500,  887500,  887500,  887500},
	{ 160000, 1012500,  987500,  962500,  950000,  950000,  937500,  925000,  912500,  900000,  887500,  887500,  887500,  887500},
	{ 100000, 1000000,  975000,  950000,  937500,  937500,  925000,  912500,  900000,  887500,  875000,  875000,  875000,  875000},
	{  50000,  975000,  950000,  925000,  912500,  912500,  900000,  887500,  875000,  862500,  862500,  850000,  850000,  850000},
};

static unsigned int int_mif_lv3_asv_volt_info[INT_DVFS_LEVEL_NR][INT_ASV_GRP_NR + 1] = {
	{ 800000, 1100000, 1075000,	1050000, 1037500,	1025000, 1012500,	1000000,  987500,  987500,  975000,  975000,	975000,	 975000},
	{ 700000, 1075000, 1050000,	1025000, 1012500,	1000000,  987500,	 975000,  962500,  962500,  950000,  950000,	950000,	 950000},
	{ 400000, 1012500,  987500,	 962500,  950000,  950000,  937500,	 925000,  912500,  900000,  887500,  887500,	887500,	 887500},
	{ 267000, 1012500,  987500,	 962500,  950000,  950000,  937500,	 925000,  912500,  900000,  887500,  887500,	887500,	 887500},
	{ 200000, 1012500,  987500,	 962500,  950000,  950000,  937500,	 925000,  912500,  900000,  887500,  887500,	887500,	 887500},
	{ 160000, 1012500,  987500,	 962500,  950000,  950000,  937500,	 925000,  912500,  900000,  887500,  887500,	887500,	 887500},
	{ 100000, 1000000,  975000,	 950000,  937500,  937500,  925000,	 912500,  900000,  887500,  875000,  875000,	875000,	 875000},
	{  50000,  975000,  950000,	 925000,  912500,  912500,  900000,	 887500,  875000,  862500,  862500,  850000,	850000,	 850000},
};

static unsigned int mif_asv_volt_info[MIF_DVFS_LEVEL_NR][MIF_ASV_GRP_NR + 1] = {
	{ 800000, 1025000, 1012500, 1012500, 1000000,  987500,  987500,  975000,  975000,  975000,  975000,  975000,  975000,  975000},
	{ 400000,  937500,  925000,  925000,  912500,  900000,  900000,  887500,  887500,  887500,  887500,  887500,  887500,  887500},
	{ 200000,  900000,  875000,  862500,  862500,  850000,  850000,  850000,  850000,  837500,  837500,  825000,  825000,  825000},
	{ 100000,  875000,  862500,  862500,  850000,  850000,  850000,  850000,  837500,  837500,  825000,  825000,  825000,  825000},
};

static unsigned int g3d_asv_volt_info[G3D_DVFS_LEVEL_NR][G3D_ASV_GRP_NR + 1] = {
	{ 532000, 1150000, 1137500, 1125000, 1100000, 1087500, 1075000, 1062500, 1050000, 1037500, 1025000, 1012500, 1000000, 1000000},
	{ 480000, 1100000, 1087500, 1075000, 1062500, 1050000, 1037500, 1025000, 1012500, 1000000,  987500,  975000,  962500,  950000},
	{ 350000,  987500,  975000,  962500,  950000,  937500,  925000,  925000,  912500,  912500,  900000,  900000,  900000,  900000},
	{ 266000,  950000,  937500,  925000,  912500,  900000,  900000,  900000,  900000,  900000,  900000,  900000,  900000,  900000},
	{ 177000,  925000,  912500,  900000,  900000,  900000,  900000,  900000,  900000,  900000,  900000,  900000,  900000,  900000},
	{  89000,  925000,  912500,  900000,  900000,  900000,  900000,  900000,  900000,  900000,  900000,  900000,  900000,  900000},
};

#endif /* EXYNOS5410_ASV_H */
