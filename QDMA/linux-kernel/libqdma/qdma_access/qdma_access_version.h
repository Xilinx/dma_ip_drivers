/*
 * Copyright(c) 2019 Xilinx, Inc. All rights reserved.
 */

#ifndef QDMA_VERSION_H_
#define QDMA_VERSION_H_


#define QDMA_VERSION_MAJOR	2019
#define QDMA_VERSION_MINOR	1
#define QDMA_VERSION_PATCH	40

#define QDMA_VERSION_STR	\
	__stringify(QDMA_VERSION_MAJOR) "." \
	__stringify(QDMA_VERSION_MINOR) "." \
	__stringify(QDMA_VERSION_PATCH)

#define QDMA_VERSION  \
	((QDMA_VERSION_MAJOR)*1000 + \
	 (QDMA_VERSION_MINOR)*100 + \
	  QDMA_VERSION_PATCH)


#endif /* COMMON_QDMA_VERSION_H_ */
