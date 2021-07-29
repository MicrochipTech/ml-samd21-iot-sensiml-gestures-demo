#ifndef SML_OUTPUT_H
#define	SML_OUTPUT_H

#include <stdint.h>

uint32_t sml_output_init(void *p_module);

uint32_t sml_output_results(uint16_t model, uint16_t classification);

#ifdef	__cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif	/* SML_OUTPUT_H */

