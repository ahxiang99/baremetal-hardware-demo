#ifndef DMA_BITFIELDS_H
#define DMA_BITFIELDS_H

#include <stdint.h>

// DMA_LISR

#define DMA_LISR_FEIF0 (1 << 0)
#define DMA_LISR_DMEIF0 (1 << 2)
#define DMA_LISR_TEIF0 (1 << 3)
#define DMA_LISR_HTIF0 (1 << 4)
#define DMA_LISR_TCIF0 (1 << 5)

#define DMA_LISR_FEIF1 (1 << 6)
#define DMA_LISR_DMEIF1 (1 << 8)
#define DMA_LISR_TEIF1 (1 << 9)
#define DMA_LISR_HTIF1 (1 << 10)
#define DMA_LISR_TCIF1 (1 << 11)

#define DMA_LISR_FEIF2 (1 << 16)
#define DMA_LISR_DMEIF2 (1 << 18)
#define DMA_LISR_TEIF2 (1 << 19)
#define DMA_LISR_HTIF2 (1 << 20)
#define DMA_LISR_TCIF2 (1 << 21)

#define DMA_LISR_FEIF3 (1 << 22)
#define DMA_LISR_DMEIF3 (1 << 24)
#define DMA_LISR_TEIF3 (1 << 25)
#define DMA_LISR_HTIF3 (1 << 26)
#define DMA_LISR_TCIF3 (1 << 27)

// DMA_HISR

#define DMA_HISR_FEIF4 (1 << 0)
#define DMA_HISR_DMEIF4 (1 << 2)
#define DMA_HISR_TEIF4 (1 << 3)
#define DMA_HISR_HTIF4 (1 << 4)
#define DMA_HISR_TCIF4 (1 << 5)

#define DMA_HISR_FEIF5 (1 << 6)
#define DMA_HISR_DMEIF5 (1 << 8)
#define DMA_HISR_TEIF5 (1 << 9)
#define DMA_HISR_HTIF5 (1 << 10)
#define DMA_HISR_TCIF5 (1 << 11)

#define DMA_HISR_FEIF6 (1 << 16)
#define DMA_HISR_DMEIF6 (1 << 18)
#define DMA_HISR_TEIF6 (1 << 19)
#define DMA_HISR_HTIF6 (1 << 20)
#define DMA_HISR_TCIF6 (1 << 21)

#define DMA_HISR_FEIF7 (1 << 22)
#define DMA_HISR_DMEIF7 (1 << 24)
#define DMA_HISR_TEIF7 (1 << 25)
#define DMA_HISR_HTIF7 (1 << 26)
#define DMA_HISR_TCIF7 (1 << 27)

// DMA_LIFCR

#define DMA_LIFCR_CFEIF0 (1 << 0)
#define DMA_LIFCR_CDMEIF0 (1 << 2)
#define DMA_LIFCR_CTEIF0 (1 << 3)
#define DMA_LIFCR_CHTIF0 (1 << 4)
#define DMA_LIFCR_CTCIF0 (1 << 5)

#define DMA_LIFCR_CFEIF1 (1 << 6)
#define DMA_LIFCR_CDMEIF1 (1 << 8)
#define DMA_LIFCR_CTEIF1 (1 << 9)
#define DMA_LIFCR_CHTIF1 (1 << 10)
#define DMA_LIFCR_CTCIF1 (1 << 11)

#define DMA_LIFCR_CFEIF2 (1 << 16)
#define DMA_LIFCR_CDMEIF2 (1 << 18)
#define DMA_LIFCR_CTEIF2 (1 << 19)
#define DMA_LIFCR_CHTIF2 (1 << 20)
#define DMA_LIFCR_CTCIF2 (1 << 21)

#define DMA_LIFCR_CFEIF3 (1 << 22)
#define DMA_LIFCR_CDMEIF3 (1 << 24)
#define DMA_LIFCR_CTEIF3 (1 << 25)
#define DMA_LIFCR_CHTIF3 (1 << 26)
#define DMA_LIFCR_CTCIF3 (1 << 27)

// DMA_HIFCR

#define DMA_HIFCR_CFEIF4 (1 << 0)
#define DMA_HIFCR_CDMEIF4 (1 << 2)
#define DMA_HIFCR_CTEIF4 (1 << 3)
#define DMA_HIFCR_CHTIF4 (1 << 4)
#define DMA_HIFCR_CTCIF4 (1 << 5)

#define DMA_HIFCR_CFEIF5 (1 << 6)
#define DMA_HIFCR_CDMEIF5 (1 << 8)
#define DMA_HIFCR_CTEIF5 (1 << 9)
#define DMA_HIFCR_CHTIF5 (1 << 10)
#define DMA_HIFCR_CTCIF5 (1 << 11)

#define DMA_HIFCR_CFEIF6 (1 << 16)
#define DMA_HIFCR_CDMEIF6 (1 << 18)
#define DMA_HIFCR_CTEIF6 (1 << 19)
#define DMA_HIFCR_CHTIF6 (1 << 20)
#define DMA_HIFCR_CTCIF6 (1 << 21)

#define DMA_HIFCR_CFEIF7 (1 << 22)
#define DMA_HIFCR_CDMEIF7 (1 << 24)
#define DMA_HIFCR_CTEIF7 (1 << 25)
#define DMA_HIFCR_CHTIF7 (1 << 26)
#define DMA_HIFCR_CTCIF7 (1 << 27)

// DMA_SCR

#define DMA_SCR_EN (1 << 0)
#define DMA_SCR_DMEIE (1 << 1)
#define DMA_SCR_TEIE (1 << 2)
#define DMA_SCR_HTIE (1 << 3)
#define DMA_SCR_TCIE (1 << 4)
#define DMA_SCR_PFCTRL (1 << 5)
#define DMA_SCR_DIR_Pos 6
#define DMA_SCR_CIRC (1 << 8)
#define DMA_SCR_PINC (1 << 9)
#define DMA_SCR_MINC (1 << 10)
#define DMA_SCR_PSIZE_Pos (1 << 11)
#define DMA_SCR_MSIZE_Pos (1 << 13)
#define DMA_SCR_PINCOS (1 << 15)
#define DMA_SCR_PL_Pos (1 << 16)
#define DMA_SCR_DBM (1 << 18)
#define DMA_SCR_CT (1 << 19)
#define DMA_SCR_PBURST_Pos (1 << 21)
#define DMA_SCR_MBURST_Pos (1 << 23)
#define DMA_SCR_CHSEL_Pos (25)

// DMA_FCR

#define DMA_FCR_FTH_Pos (1 << 0)
#define DMA_FCR_FDMDIS (1 << 2)
#define DMA_FCR_FS_Pos (1 << 3)
#define DMA_FCR_FEIE (1 << 7)
#

#endif