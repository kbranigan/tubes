/*****************************************************************************/
/*  LibDWG - Free DWG read-only library                                      */
/*  Copyright (C) 2008, 2009 Free Software Foundation, Inc.                  */
/*                                                                           */
/*  This library is free software, licensed under the terms of the GNU       */
/*  General Public License as published by the Free Software Foundation,     */
/*  either version 3 of the License, or (at your option) any later version.  */
/*  You should have received a copy of the GNU General Public License        */
/*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    */
/*****************************************************************************/

// Cxefa interna inkluziv-dosiero de la biblioteko.

#ifndef DVG_H
#define DVG_H

#ifdef __cplusplus
extern "C"
{
#endif

/**
 Supertipoj de objektoj kiuj ekzistas en dvg-dosieroj.
 */
	typedef enum DVG_OBJEKTO_SUPERTIPO
	{
		DVG_OST_NEKONATAJXO,
		DVG_OST_ESTAJXO,
		DVG_OST_ORDINARAJXO
	} Dvg_Objekto_Supertipo;

/**
 Tipoj de objektoj kiuj ekzistas en dvg-dosieroj.
 */
	typedef enum DVG_OBJEKTO_TIPO
	{
		DVG_OT_NEUZATA = 0,
		DVG_OT_TEXT = 1,
		DVG_OT_ATTRIB = 2,
		DVG_OT_ATTDEF = 3,
		DVG_OT_BLOCK = 4,
		DVG_OT_ENDBLK = 5,
		DVG_OT_SEQEND = 6,
		DVG_OT_INSERT = 7,
		DVG_OT_MINSERT = 8,
		DVG_OT_VERTEX_2D = 10,
		DVG_OT_VERTEX_3D = 11,
		DVG_OT_VERTEX_MESH = 12,
		DVG_OT_VERTEX_PFACE = 13,
		DVG_OT_VERTEX_PFACE_FACE = 14,
		DVG_OT_POLYLINE_2D = 15,
		DVG_OT_POLYLINE_3D = 16,
		DVG_OT_ARC = 17,
		DVG_OT_CIRCLE = 18,
		DVG_OT_LINE = 19,
		DVG_OT_POINT = 27,
		DVG_OT_ELLIPSE = 35,
		DVG_OT_RAY = 40,
		DVG_OT_XLINE = 41,
		DVG_OT_DICTIONARY = 42,
		DVG_OT_MTEXT = 44,
		DVG_OT_BLOCK_CONTROL = 48,
		DVG_OT_BLOCK_HEADER = 49,
		DVG_OT_LAYER_CONTROL = 50,
		DVG_OT_LAYER = 51,
		DVG_OT_SHAPEFILE_CONTROL = 52,
		DVG_OT_SHAPEFILE = 53,
		DVG_OT_LINETYPE_CONTROL = 56,
		DVG_OT_LTYPE = 56,
		DVG_OT_VIEW_CONTROL = 60,
		DVG_OT_UCS_CONTROL = 62,
		DVG_OT_TABLE_VIEWPORT = 64,
		DVG_OT_VPORT = 64,
		DVG_OT_TABLE_APPID = 66,
		DVG_OT_APPID = 67,
		DVG_OT_DIMSTYLE_CONTROL = 68,
		DVG_OT_DIMSTYLE = 69,
		DVG_OT_VIEWPORT_ENTITY_CONTROL = 70,
		DVG_OT_MLINESTYLE = 73,
		DVG_OT_LWPOLYLINE = 77,
		DVG_OT_FREMDA = 79
	} Dvg_Objekto_Tipo;

/**
 Strukturo por traktiloj.
 */
	typedef struct _dvg_traktilo
	{
		unsigned char kodo;
		unsigned char kiom;
		long unsigned int valoro;
	} Dvg_Traktilo;

/**
 Strukturo por 2D punktoj.
 */
	typedef struct _Punkto_2D
	{
		double x;
		double y;
	} Punkto_2D;

/**
 Strukturo por 3D punktoj.
 */
	typedef struct _Punkto_3D
	{
		double x;
		double y;
		double z;
	} Punkto_3D;

/* OBJEKTOJ *******************************************************************/
/**
 Strukturo por tekstoj - NEUZATA (0)
 */
	typedef int Dvg_Estajxo_NEUZATA;

/**
 Strukturo por tekstoj - TEXT (1)
 */
	typedef struct _dvg_estajxo_TEXT
	{
		unsigned char datumindik;
		double levigxo;
		double x0;
		double y0;
		struct
		{
			double x;
			double y;
			unsigned int h;
			unsigned int v;
		} gxisrandigo;
		struct
		{
			double x;
			double y;
			double z;
		} forpusxigo;
		double dikeco;
		double klinang;
		double turnang;
		double alteco;
		double largxfaktoro;
		char *teksto;
		unsigned int generacio;
	} Dvg_Estajxo_TEXT;

/**
 Strukturo por atributoj - ATTRIB (2)
 */
	typedef struct _dvg_estajxo_ATTRIB
	{
		unsigned char datumindik;
		double levigxo;
		double x0;
		double y0;
		struct
		{
			double x;
			double y;
			unsigned int h;
			unsigned int v;
		} gxisrandigo;
		struct
		{
			double x;
			double y;
			double z;
		} forpusxigo;
		double dikeco;
		double klinang;
		double turnang;
		double alteco;
		double largxfaktoro;
		unsigned char *teksto;
		unsigned int generacio;
		unsigned char *etikedo;
		unsigned int kamplong; //neuzita
		unsigned char indikiloj;
	} Dvg_Estajxo_ATTRIB;

/**
 Strukturo por atributo-difinoj - ATTDEF (3)
 */
	typedef struct _dvg_estajxo_ATTDEF
	{
		unsigned char datumindik;
		double levigxo;
		double x0;
		double y0;
		struct
		{
			double x;
			double y;
			unsigned int h;
			unsigned int v;
		} gxisrandigo;
		struct
		{
			double x;
			double y;
			double z;
		} forpusxigo;
		double dikeco;
		double klinang;
		double turnang;
		double alteco;
		double largxfaktoro;
		unsigned char *teksto;
		unsigned int generacio;
		unsigned char *etikedo;
		unsigned int kamplong; //neuzita
		unsigned char indikiloj;
		unsigned char *invitilo;
	} Dvg_Estajxo_ATTDEF;

/**
 Strukturo por blokoj - BLOCK (4)
 */
	typedef struct _dvg_estajxo_BLOCK
	{
		unsigned char *nomo;
	} Dvg_Estajxo_BLOCK;

/**
 Strukturo por blokfinoj - ENDBLK (5)
 */
	typedef struct _dvg_estajxo_ENDBLK
	{
	} Dvg_Estajxo_ENDBLK;

/**
 Strukturo por enmetoj - INSERT (7)
 */
	typedef struct _dvg_estajxo_INSERT
	{
		double x0;
		double y0;
		double z0;
		unsigned char skalindik;
		struct
		{
			double x;
			double y;
			double z;
		} skalo;
		double turnang;
		struct
		{
			double x;
			double y;
			double z;
		} forpusxigo;
		unsigned char kun_attrib;
	} Dvg_Estajxo_INSERT;

/**
 Strukturo por multoblaj enmetoj - MINSERT (7)
 */
	typedef struct _dvg_estajxo_MINSERT
	{
		double x0;
		double y0;
		double z0;
		unsigned char skalindik;
		struct
		{
			double x;
			double y;
			double z;
		} skalo;
		double turnang;
		struct
		{
			double x;
			double y;
			double z;
		} forpusxigo;
		unsigned char kun_attrib;
		struct
		{
			unsigned int kiom;
			double dx;
		} kol;
		struct
		{
			unsigned int kiom;
			double dy;
		} lin;
	} Dvg_Estajxo_MINSERT;

/**
 Strukturo por verticoj - VERTEX_2D (10)
 */
	typedef struct _dvg_estajxo_VERTEX_2D
	{
		unsigned char indikiloj;
		double x0;
		double y0;
		double z0;
		double eklargxo;
		double finlargxo;
		double protuberanco;
		double tangxdir;
	} Dvg_Estajxo_VERTEX_2D;

/**
 Strukturo por verticoj - VERTEX_3D (11)
 */
	typedef struct _dvg_estajxo_VERTEX_3D
	{
		unsigned char indikiloj;
		double x0;
		double y0;
		double z0;
	} Dvg_Estajxo_VERTEX_3D;

/**
 Strukturo por verticoj - VERTEX_PFACE_FACE (14)
 */
	typedef struct _dvg_estajxo_VERTEX_PFACE_FACE
	{
		unsigned int vertind[4];
	} Dvg_Estajxo_VERTEX_PFACE_FACE;

/**
 Strukturo por:  2D POLYLINE (15)
 */
	typedef struct _dvg_estajxo_POLYLINE_2D
	{
		unsigned int indikiloj;
		unsigned int kurbtipo;
		double eklargxo;
		double finlargxo;
		double dikeco;
		double levigxo;
		struct
		{
			double x;
			double y;
			double z;
		} forpusxigo;
	} Dvg_Estajxo_POLYLINE_2D;

/**
 Strukturo por:  3D POLYLINE (16)
 */
	typedef struct _dvg_estajxo_POLYLINE_3D
	{
		unsigned char indikiloj_1;
		unsigned char indikiloj_2;
	} Dvg_Estajxo_POLYLINE_3D;

/**
 Strukturo por arkoj - ARC (17)
 */
	typedef struct _dvg_estajxo_ARC
	{
		double x0;
		double y0;
		double z0;
		double radiuso;
		double dikeco;
		struct
		{
			double x;
			double y;
			double z;
		} forpusxigo;
		double ekangulo;
		double finangulo;
	} Dvg_Estajxo_ARC;

/**
 Strukturo por cirkloj - CIRCLE (18)
 */
	typedef struct _dvg_estajxo_CIRCLE
	{
		double x0;
		double y0;
		double z0;
		double radiuso;
		double dikeco;
		struct
		{
			double x;
			double y;
			double z;
		} forpusxigo;
	} Dvg_Estajxo_CIRCLE;

/**
 Strukturo por linioj - LINE (19)
 */
	typedef struct _dvg_estajxo_LINE
	{
		unsigned char nur_2D;
		double x0;
		double x1;
		double y0;
		double y1;
		double z0;
		double z1;
		double dikeco;
		struct
		{
			double x;
			double y;
			double z;
		} forpusxigo;
	} Dvg_Estajxo_LINE;

/**
 Strukturo por punktoj - POINT (27)
 */
	typedef struct _dvg_estajxo_POINT
	{
		double x0;
		double y0;
		double z0;
		double dikeco;
		struct
		{
			double x;
			double y;
			double z;
		} forpusxigo;
		double x_ang;
	} Dvg_Estajxo_POINT;

/**
 Strukturo por elipsoj - ELLIPSE (35)
 */
	typedef struct _dvg_estajxo_ELLIPSE
	{
		double x0;
		double y0;
		double z0;
		double x1;
		double y1;
		double z1;
		struct
		{
			double x;
			double y;
			double z;
		} forpusxigo;
		double radiusproporcio;
		double ekangulo;
		double finangulo;
	} Dvg_Estajxo_ELLIPSE;

/**
 Strukturo por radioj - RAY (40)
 */
	typedef struct _dvg_estajxo_RAY
	{
		double x0;
		double y0;
		double z0;
		double x1;
		double y1;
		double z1;
	} Dvg_Estajxo_RAY;

/**
 Strukturo por tekstoj - MTEXT (44)
 */
	typedef struct _dvg_estajxo_MTEXT
	{
		double x0;
		double y0;
		double z0;
		struct
		{
			double x;
			double y;
			double z;
		} forpusxigo;
		struct
		{
			double x;
			double y;
			double z;
		} X_akso; // Unueca vektoro por la direkto
		double reflargxeco; // referenca largxeco
		double alteco; // de la teksto
		unsigned int kunmeto; // simila al gxisrandigo
		unsigned int direkto; //maldekstre-dekstren, ktp.
		struct
		{
			double alteco;
			double largxeco;
		} kadro;
		char *teksto;
		unsigned int linispaco_stilo;
		unsigned int linispaco_faktoro;
		unsigned char ia_bito; // ne konata
	} Dvg_Estajxo_MTEXT;

/**
 Strukturo por vortaroj - DICTIONARY (42)
 */
	typedef struct _dvg_ordinarajxo_DICTIONARY
	{
		unsigned int kiom;
		unsigned int imito;
		unsigned char posedanto;
		char **nomo;
	} Dvg_Ordinarajxo_DICTIONARY;

/**
 Strukturo por bloko-kontrolo - BLOCK_CONTROL (48)
 */
	typedef struct _dvg_ordinarajxo_BLOCK_CONTROL
	{
		unsigned int kiom;
	} Dvg_Ordinarajxo_BLOCK_CONTROL;

/**
 Strukturo por bloko-kapoj - BLOCK_HEADER (49)
	typedef struct _dvg_ordinarajxo_BLOCK_HEADER
	{
		char *nomo;
		// Kompletende...
	} Dvg_Ordinarajxo_BLOCK_HEADER;
 */

/**
 Strukturo por tavoloj - LAYER (51)
 */
	typedef struct _dvg_ordinarajxo_LAYER
	{
		char *nomo;
		unsigned char bito64;
		unsigned int xrefi;
		unsigned char xrefdep;
		unsigned int ecoj;
		unsigned int koloro;
	} Dvg_Ordinarajxo_LAYER;

/**
 Strukturo por simplaj plurlateroj - LWPOLYLINE (77)
 */
	typedef struct _dvg_estajxo_LWPOLYLINE
	{
		unsigned indikiloj;
		double largxeco;
		double levigxo;
		double dikeco;
		struct
		{
			double x;
			double y;
			double z;
		} forpusxigo;

		long unsigned vertico_kiom; 
		long unsigned rondigo_kiom;
		long unsigned linilargxo_kiom;

		struct
		{
			double x;
			double y;
		} *vertico;

		double *rondigo; // 'Bulge'

		struct
		{
			double ek;
			double fin;
		} *linilargxo;

	} Dvg_Estajxo_LWPOLYLINE;

/**
 Strukturo por arangxo - LAYOUT (502?)
 */
	typedef struct _dvg_ordinarajxo_LAYOUT
	{
		struct
		{
			char *agordo;
			char *printilo;
			unsigned int indikiloj;
			double maldekstre;
			double malsupre;
			double dekstre;
			double supre;
			double largxeco;
			double alteco;
			char *grandeco;
			double dx;
			double dy;
			unsigned int unuoj;
			unsigned int rotacio;
			unsigned int tipo;
			double x_min;
			double y_min;
			double x_maks;
			double y_maks;
			char *nomo;
			struct
			{
				double A; // A:B (ekz: 1:10, 1:2, 50:1, ktp)
				double B;
				unsigned int tipo;
				double faktoro;
			} skalo;
			char *stilfolio;
			double x0;
			double y0;
		} pagxo;

		char *nomo;
		unsigned int ordo;
		unsigned int indikiloj;
		double x0;
		double y0;
		double z0;
		double x_min;
		double y_min;
		double x_maks;
		double y_maks;
		struct
		{
			double x0;
			double y0;
			double z0;
		} enmeto;
		struct
		{
			double x0;
			double y0;
			double z0;
		} akso_X;
		struct
		{
			double x0;
			double y0;
			double z0;
		} akso_Y;
		double levigxo;
		unsigned int rigardtipo;
		struct
		{
			double x_min;
			double y_min;
			double z_min;
			double x_maks;
			double y_maks;
			double z_maks;
		} limo;
	} Dvg_Ordinarajxo_LAYOUT;

/* OBJEKTOJ - FINO ************************************************************/

/**
 Strukturo de rikordoj por atributoj de estajxoj.
 */
	typedef struct _dvg_objekto_estajxo
	{
		unsigned char bildo_ekzistas;
		long unsigned int bildo_kiom;
		unsigned char *bildo;

		unsigned char regximo;
		long unsigned int reagilo_kiom;
		unsigned char senligiloj;
		unsigned int koloro;
		double linitiposkalo;
		unsigned char linitipo;
		unsigned char printstilo;
		unsigned int malvidebleco;
		unsigned char linidikeco;

		union
		{
			Dvg_Estajxo_NEUZATA *NEUZATA;
			Dvg_Estajxo_TEXT *TEXT;
			Dvg_Estajxo_ATTRIB *ATTRIB;
			Dvg_Estajxo_ATTDEF *ATTDEF;
			Dvg_Estajxo_BLOCK *BLOCK;
			Dvg_Estajxo_ENDBLK *ENDBLK;
			Dvg_Estajxo_INSERT *INSERT;
			Dvg_Estajxo_MINSERT *MINSERT;
			Dvg_Estajxo_VERTEX_2D *VERTEX_2D;
			Dvg_Estajxo_VERTEX_3D *VERTEX_3D;
			Dvg_Estajxo_VERTEX_PFACE_FACE *VERTEX_PFACE_FACE;
			Dvg_Estajxo_POLYLINE_2D *POLYLINE_2D;
			Dvg_Estajxo_POLYLINE_3D *POLYLINE_3D;
			Dvg_Estajxo_ARC *ARC;
			Dvg_Estajxo_CIRCLE *CIRCLE;
			Dvg_Estajxo_LINE *LINE;
			Dvg_Estajxo_POINT *POINT;
			Dvg_Estajxo_ELLIPSE *ELLIPSE;
			Dvg_Estajxo_RAY *RAY;
			Dvg_Estajxo_MTEXT *MTEXT;
			Dvg_Estajxo_LWPOLYLINE *LWPOLYLINE;
		} tio;

		unsigned int traktref_kiom;
		Dvg_Traktilo *traktref;
	} Dvg_Objekto_Estajxo;

/**
 Strukturo de rikordoj por atributoj de ordinaraj objektoj.
 */
	typedef struct _dvg_objekto_ordinarajxo
	{
		long unsigned int reagilo_kiom;

		union
		{
			Dvg_Ordinarajxo_LAYER *LAYER;
			Dvg_Ordinarajxo_DICTIONARY *DICTIONARY;
			Dvg_Ordinarajxo_BLOCK_CONTROL *BLOCK_CONTROL;
			//Dvg_Ordinarajxo_BLOCK_HEADER *BLOCK_HEADER;
			Dvg_Ordinarajxo_LAYOUT *LAYOUT;
		} tio;

		unsigned int traktref_kiom;
		Dvg_Traktilo *traktref;
	} Dvg_Objekto_Ordinarajxo;

/**
 Gxenerala strukturo de rikordoj por objektoj.
 */
	typedef struct _dvg_objekto
	{
		unsigned grandeco;
		unsigned tipo;

		long unsigned bitgrandeco;
		Dvg_Traktilo traktilo;

		unsigned kromdat_kiom;
		Dvg_Traktilo kromdat_trakt;
		unsigned char *kromdat;

		Dvg_Objekto_Supertipo supertipo;
		union
		{
			Dvg_Objekto_Estajxo *estajxo;
			Dvg_Objekto_Ordinarajxo *ordinarajxo;
			unsigned char *nekonatajxo;
		} tio;

		unsigned ckr;

		/* Kromaj variabloj
		 */
		long unsigned trakt;
	} Dvg_Objekto;

/**
 Strukturo de rikordoj por klasoj.
 */
	typedef struct _dvg_klaso
	{
		unsigned int numero;
		unsigned int versio;
		unsigned char *apnomo;
		unsigned char *cpliplinomo;
		unsigned char *dxfnomo;
		unsigned char estisfantomo;
		unsigned int eroid;
	} Dvg_Klaso;

/**
 Dvg_Cxeno sama kiel Bit_Cxeno, en "bite.h"
 */
	typedef struct _dvg_cxeno
	{
		unsigned char *cxeno;
		long unsigned int kiom;
		long unsigned int bajto;
		unsigned char bito;
	} Dvg_Cxeno;

/**
 Cxefa strukturo de la dvg-datenaro.
 */
	typedef struct _dvg_strukturo
	{
		struct
		{
			char versio[6];
			unsigned int kodpagxo;
			unsigned int sekcio_kiom;
			struct
			{
				unsigned char numero;
				long unsigned int adresilo;
				long unsigned int grandeco;
			} sekcio[6];
		} kapo;

#		define DVG_NEKONATA1_KIOM 123
		Dvg_Cxeno nekonata1;

		Dvg_Cxeno bildo;

#		define DVG_KIOM_VARIABLOJ 233
		union
		{
			unsigned char bitoko;
			unsigned int dubitoko;
			long unsigned int kvarbitoko;
			double duglitajxo;
			double xyz[3];
			double xy[2];
			unsigned char *teksto;
			Dvg_Traktilo traktilo;
		} var[DVG_KIOM_VARIABLOJ];

		Dvg_Klaso *klaso;
		unsigned int klaso_kiom;

		Dvg_Objekto *objekto;
		long unsigned int objekto_kiom;

		struct
		{
			unsigned char nekonatajxo[6];
			struct
			{
				int kiom;
				unsigned char cxeno[4];
			} traktrik[14];
		} duakapo;

		long unsigned int mezuro;

		unsigned int dvg_ot_layout;

	} Dvg_Strukturo;

/*--------------------------------------------------
 * Funkcioj
 */
int dvg_legi_dosiero (char *dosiernomo, Dvg_Strukturo *skt);

int dvg_skribi_dosiero (char *dosiernomo, Dvg_Strukturo *strukt);

unsigned char * dvg_bmp_kiu (Dvg_Strukturo *stk, long *kiom);

double dvg_var_modelo_x_min (Dvg_Strukturo *stk);
double dvg_var_modelo_x_maks (Dvg_Strukturo *stk);
double dvg_var_modelo_y_min (Dvg_Strukturo *stk);
double dvg_var_modelo_y_maks (Dvg_Strukturo *stk);
double dvg_var_modelo_z_min (Dvg_Strukturo *stk);
double dvg_var_modelo_z_maks (Dvg_Strukturo *stk);
double dvg_var_modelo_pagxo_x_min (Dvg_Strukturo *stk);
double dvg_var_modelo_pagxo_x_maks (Dvg_Strukturo *stk);
double dvg_var_modelo_pagxo_y_min (Dvg_Strukturo *stk);
double dvg_var_modelo_pagxo_y_maks (Dvg_Strukturo *stk);

long unsigned dvg_objekto_kiom (Dvg_Strukturo *stk);
unsigned dvg_objekto_supertipo (Dvg_Strukturo *stk, long unsigned tiu);
unsigned dvg_objekto_tipo (Dvg_Strukturo *stk, long unsigned tiu);

unsigned char dvg_estajxo_regximo (Dvg_Strukturo *stk, long unsigned tiu);

double dvg_punkto_x0 (Dvg_Strukturo *stk, long unsigned tiu);
double dvg_punkto_y0 (Dvg_Strukturo *stk, long unsigned tiu);
double dvg_punkto_z0 (Dvg_Strukturo *stk, long unsigned tiu);

double dvg_linio_x0 (Dvg_Strukturo *stk, long unsigned tiu);
double dvg_linio_y0 (Dvg_Strukturo *stk, long unsigned tiu);
double dvg_linio_z0 (Dvg_Strukturo *stk, long unsigned tiu);
double dvg_linio_x1 (Dvg_Strukturo *stk, long unsigned tiu);
double dvg_linio_y1 (Dvg_Strukturo *stk, long unsigned tiu);
double dvg_linio_z1 (Dvg_Strukturo *stk, long unsigned tiu);

int dvg_liniaro_cxu_fermita (Dvg_Strukturo *stk, long unsigned tiu);
long unsigned dvg_liniaro_vertico_kiom (Dvg_Strukturo *stk, long unsigned tiu);
double dvg_liniaro_xi (Dvg_Strukturo *stk, long unsigned tiu, long unsigned vertico);
double dvg_liniaro_yi (Dvg_Strukturo *stk, long unsigned tiu, long unsigned vertico);
long unsigned dvg_liniaro_rondigo_kiom (Dvg_Strukturo *stk, long unsigned tiu);
double dvg_liniaro_ri (Dvg_Strukturo *stk, long unsigned tiu, long unsigned vertico);

double dvg_cirklo_x0 (Dvg_Strukturo *stk, long unsigned tiu);
double dvg_cirklo_y0 (Dvg_Strukturo *stk, long unsigned tiu);
double dvg_cirklo_z0 (Dvg_Strukturo *stk, long unsigned tiu);
double dvg_cirklo_r (Dvg_Strukturo *stk, long unsigned tiu);

double dvg_arko_x0 (Dvg_Strukturo *stk, long unsigned tiu);
double dvg_arko_y0 (Dvg_Strukturo *stk, long unsigned tiu);
double dvg_arko_z0 (Dvg_Strukturo *stk, long unsigned tiu);
double dvg_arko_r (Dvg_Strukturo *stk, long unsigned tiu);
double dvg_arko_a (Dvg_Strukturo *stk, long unsigned tiu); //ekangulo, radianoj
double dvg_arko_b (Dvg_Strukturo *stk, long unsigned tiu); //finangulo, radianoj

char * dvg_teksto (Dvg_Strukturo *stk, long unsigned tiu);
double dvg_teksto_alteco (Dvg_Strukturo *stk, long unsigned tiu);
double dvg_teksto_x0 (Dvg_Strukturo *stk, long unsigned tiu);
double dvg_teksto_y0 (Dvg_Strukturo *stk, long unsigned tiu);

char * dvg_atdif (Dvg_Strukturo *stk, long unsigned tiu);
char * dvg_atdif_etikedo (Dvg_Strukturo *stk, long unsigned tiu);
double dvg_atdif_alteco (Dvg_Strukturo *stk, long unsigned tiu);
double dvg_atdif_x0 (Dvg_Strukturo *stk, long unsigned tiu);
double dvg_atdif_y0 (Dvg_Strukturo *stk, long unsigned tiu);

char * dvg_atrib (Dvg_Strukturo *stk, long unsigned tiu);
char * dvg_atrib_etikedo (Dvg_Strukturo *stk, long unsigned tiu);
double dvg_atrib_alteco (Dvg_Strukturo *stk, long unsigned tiu);
double dvg_atrib_x0 (Dvg_Strukturo *stk, long unsigned tiu);
double dvg_atrib_y0 (Dvg_Strukturo *stk, long unsigned tiu);

char * dvg_mteksto (Dvg_Strukturo *stk, long unsigned tiu);
char * dvg_mteksto_unikse (Dvg_Strukturo *stk, long unsigned tiu); // uzi '\n' por aline-fino
double dvg_mteksto_alteco (Dvg_Strukturo *stk, long unsigned tiu); // alteco de la teksto
double dvg_mteksto_x0 (Dvg_Strukturo *stk, long unsigned tiu);
double dvg_mteksto_y0 (Dvg_Strukturo *stk, long unsigned tiu);
double dvg_mteksto_dx (Dvg_Strukturo *stk, long unsigned tiu); // kadro-largxeco
double dvg_mteksto_dy (Dvg_Strukturo *stk, long unsigned tiu); // kadro-alteco

#ifdef __cplusplus
}
#endif

#endif
