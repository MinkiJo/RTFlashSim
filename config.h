
#ifndef CONFIG_H_
#define CONFIG_H_

#define TRUE 1
#define FALSE 0

#define K (1024)
#define M (1024*K)
#define G (1024*M)
#define T (1024L*G)
#define P (1024L*T)
#define MILI (1000000)


#define PPB 32
#define NOB (32)
#define NOP (PPB*NOB)
#define NOC 1


#define RNOB (NOB * NOC)
#define RNOP (PPB * RNOB)

#define PAGESIZE 4
#define BLOCKSIZE (PAGESIZE * PPB)
#define TOTALSIZE (BLOCKSIZE * NOB)
#define RTOTALSIZE (TOTALSIZE * NOC)


#define ERASE 0
#define VALID 1
#define INVALID 2


/* for real time flash */
#define CUTIL 0.5
#define DUTIL (int)(CUTIL * RNOP)
#define ALPHA (int)(PPB * CUTIL)


#define OP 0.5
#endif