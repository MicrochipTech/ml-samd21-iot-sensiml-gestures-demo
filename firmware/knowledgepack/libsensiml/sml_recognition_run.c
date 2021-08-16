#include "app_config.h"
#include "kb.h"
#include "sml_output.h"
#include "sml_recognition_run.h"
#ifdef SML_USE_TEST_DATA
#include "testdata.h"
int td_index = 0;
#endif //SML_USE_TEST_DATA

#define KB_MODEL_j1_rank_0_INDEX KB_MODEL_train_with_unknown_rank_0_INDEX

int sml_recognition_run(snsr_data_t *data, int num_sensors)
{
    int ret;
//    uint32_t runtime = read_timer_ms();
    ret = kb_run_model((SENSOR_DATA_T *)data, num_sensors, KB_MODEL_j1_rank_0_INDEX);
//    runtime = read_timer_ms() - runtime;
    if (ret >= 0){
        sml_output_results(KB_MODEL_j1_rank_0_INDEX, ret);
        kb_reset_model(0);
//        printf("Classification completed in %lums\n", runtime);
    };

    return ret;
}
