#include "app_config.h"
#include "kb.h"
#include "sml_output.h"
#ifdef SML_USE_TEST_DATA
#include "testdata.h"
int td_index = 0;
#endif //SML_USE_TEST_DATA

#define KB_MODEL_j1_rank_0_INDEX KB_MODEL_train_with_unknown_rank_0_INDEX

int sml_recognition_run(SNSR_DATA_TYPE *data, int num_sensors)
{
    int ret;
    ret = kb_run_model((SENSOR_DATA_T *)data, num_sensors, KB_MODEL_j1_rank_0_INDEX);
    if (ret >= 0){
        sml_output_results(KB_MODEL_j1_rank_0_INDEX, ret);
        kb_reset_model(0);
    };

    return ret;
}
