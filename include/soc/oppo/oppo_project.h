/*
 *
 * yixue.ge add for oppo project
 *
 *
 */
#ifndef _OPPO_PROJECT_H_
#define _OPPO_PROJECT_H_

enum{
    HW_VERSION__UNKNOWN,
    HW_VERSION__10,		//EVB
    HW_VERSION__11,		//T0
    HW_VERSION__12,		//EVT-1
    HW_VERSION__13,		//EVT-2
    HW_VERSION__14,		//DVT-1
    HW_VERSION__15,		//DVT-2
    HW_VERSION__16,		//PVT
    HW_VERSION__17,		//MP
    HW_VERSION_MAX,
};

enum{
    RF_VERSION__UNKNOWN,
    RF_VERSION__11,
    RF_VERSION__12,
    RF_VERSION__13,
    RF_VERSION__21,
    RF_VERSION__22,
    RF_VERSION__23,
    RF_VERSION__31,
    RF_VERSION__32,
    RF_VERSION__33,
};

#define GET_PCB_VERSION() (get_PCB_Version())
#define GET_PCB_VERSION_STRING() (get_PCB_Version_String())

#define GET_MODEM_VERSION() (get_Modem_Version())
#define GET_OPERATOR_VERSION() (get_Operator_Version())

enum OPPO_PROJECT {
    OPPO_UNKOWN = 0,
	OPPO_17331 = 17331,
	OPPO_17197 = 17197,
	OPPO_17061 = 17061,
	OPPO_17175 = 17175,
    OPPO_17311 = 17311,
    OPPO_17321 = 17321,
    OPPO_17323 = 17323,
    OPPO_17325 = 17325,
    OPPO_17031 = 17031,
    OPPO_17032 = 17032,
    OPPO_17051 = 17051,
    OPPO_17052 = 17052,
    OPPO_17053 = 17053,
    OPPO_17055 = 17055,
    OPPO_17071 = 17071,
    OPPO_17073 = 17073,//17073 custom
    OPPO_17371 = 17371,
    OPPO_17373 = 17373,
    OPPO_17101 = 17101,
    OPPO_17102 = 17102,
    OPPO_17103 = 17103,
    OPPO_17105 = 17105,
    OPPO_17111 = 17111,
    OPPO_17113 = 17113,
    OPPO_17181 = 17181,
    OPPO_17182 = 17182,
    OPPO_17307 = 17307,
    OPPO_17309 = 17309,
    OPPO_17310 = 17310,
    OPPO_17381 = 17381,
    OPPO_18311 = 18311,
    OPPO_18011 = 18011,
    OPPO_18151 = 18151,
    OPPO_18531 = 18531,
    OPPO_18601 = 18601,
    OPPO_18611 = 18611,
    OPPO_17065 = 17065,
    OPPO_17066 = 17066,
};

enum OPPO_OPERATOR {
    OPERATOR_UNKOWN                   	= 0,
    OPERATOR_17131_CHINA_OPEN_MARKET    = 1,
    OPERATOR_17198_MOBILE             	= 2,
    OPERATOR_CHINA_UNICOM             	= 3,
    OPERATOR_CHINA_TELECOM            	= 4,
    OPERATOR_17335_17197_ALL_BAND		= 5,
    OPERATOR_17332_ALL_BAND_HIGH_CONFIG = 6,
    OPERATOR_FOREIGN_OPEN_MARKET     	= 7,
    OPERATOR_17331_ASIA             	= 8,
    OPERATOR_17199_TAIWAN         		= 9,
    OPERATOR_FOREIGN_INDIA            	= 10,
    OPERATOR_FOREIGN_MALAYSIA         	= 11,
    OPERATOR_FOREIGN_EU               	= 12,
    OPERATOR_17331_EVB		      	  	= 13,
    OPERATOR_17337_ASIA				  	= 14,
    OPERATOR_17338_INDIA			  	= 15,
    OPERATOR_17340_ALL_BAND			  	= 16,
    OPERATOR_17339_ALL_BAND_HIGH_CONFIG = 17,
    OPERATOR_17061_CHINA				= 18,
    OPERATOR_17061_MOBILE				= 19,
    OPERATOR_17061_ASIA					= 20,
    OPERATOR_17062_ASIA_SIMPLE			= 21,
    OPERATOR_17063_TAIAO				= 22,
    OPERATOR_17175_ALL_BAND				= 23,
    OPERATOR_17176_MOBILE				= 24,
    OPERATOR_17179_FOREIGN				= 25,
    OPERATOR_17180_FOREIGN				= 26,
    OPERATOR_17341_INDIA				= 27,
    OPERATOR_17171_ASIA_SIMPLE			= 28,
    OPERATOR_17172_ASIA					= 29,
    OPERATOR_17173_ALL_BAND				= 30,
    OPERATOR_17065_INDIA				= 31,
    OPERATOR_17067_CHINA				= 32,
    OPERATOR_17066_INDIA_HIGH_CONFIG	= 33,
    OPERATOR_FOREIGN_TAIWAN_17111       = 34,
    OPERATOR_ALL_TELECOM_CARRIER_17111  = 35,
    OPERATOR_ALL_MOBILE_CARRIER_17113   = 36,
    OPERATOR_18311_ASIA					= 37,
    OPERATOR_18312_INDIA				= 38,
    OPERATOR_18313_ASIA_ALL_BAND		= 39,
    OPERATOR_18011_CHINA_ALL_BAND		= 40,
    OPERATOR_18012_CMCC					= 41,
    OPERATOR_18317_VIETNAM				= 42,
    OPERATOR_18312_INDIA_EVB			= 43,
    OPERATOR_18012_CMCC_EVB				= 44,
    OPERATOR_18318_VIETNAM				= 45,
    OPERATOR_18328_ASIA_SIMPLE_NORMALCHG = 46,
    OPERATOR_18311_ASIA_2ND_RESOURCE	= 47,
    OPERATOR_18012_CMCC_2ND_RESOURCE	= 48,
    OPERATOR_18013_CHINA_ALL_BAND_FINGER = 49,
    OPERATOR_18015_CMCC_FINGER			= 50,
    OPERATOR_18601_REALME_INDIA                      = 100,
    OPERATOR_18601_REALME_ALL_BAND                   = 101,
    OPERATOR_18601_REALME_ALL_BAND_VIETNAM          = 102,
    OPERATOR_18601_REALME_INDIA_P70                  = 103,
    OPERATOR_18601_REALME_ALL_BAND_P70               = 104,
    OPERATOR_18601_REALME_ALL_BAND_VIETNAM_P70      = 105,
	OPERATOR_18601_REALME_ALL_BAND_VIETNAM_3X64      = 106,
    /*for realme, start from 56*/
    OPERATOR_18531_ASIA_SIMPLE			= 56,
    OPERATOR_18531_ASIA					= 57,
    OPERATOR_18531_All_BAND				= 58,
    OPERATOR_18611_REALME_INDIA                 = 53,
    OPERATOR_18611_REALME_ALL_BAND              = 54,
    OPERATOR_18611_REALME_VIETNAM_ALL_BAND      = 55,
    OPERATOR_18611_REALME_INDIA_DDR4            = 56,
};

enum{
    SMALLBOARD_VERSION__0,
    SMALLBOARD_VERSION__1,
    SMALLBOARD_VERSION__2,
    SMALLBOARD_VERSION__3,
    SMALLBOARD_VERSION__4,
    SMALLBOARD_VERSION__5,
    SMALLBOARD_VERSION__6,
    SMALLBOARD_VERSION__UNKNOWN = 100,
};

typedef enum OPPO_PROJECT OPPO_PROJECT;

typedef struct
{
    unsigned int    nProject;
    unsigned int    nModem;
    unsigned int    nOperator;
    unsigned int    nPCBVersion;
} ProjectInfoCDTType;

//unsigned int init_project_version(void);
unsigned int get_project(void);
unsigned int is_project(OPPO_PROJECT project );
unsigned int get_PCB_Version(void);
unsigned int get_Modem_Version(void);
unsigned int get_Operator_Version(void);

#endif
