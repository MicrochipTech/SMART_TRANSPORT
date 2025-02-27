#ifndef __MODEL_JSON_H__
#define __MODEL_JSON_H__

const char recognition_model_string_json[] = {"{\"NumModels\":1,\"ModelIndexes\":{\"0\":\"MOTOR_V_D_RANK_0\"},\"ModelDescriptions\":[{\"Name\":\"MOTOR_V_D_RANK_0\",\"ClassMaps\":{\"1\":\"Blade\",\"2\":\"Off\",\"3\":\"On\",\"0\":\"Unknown\"},\"ModelType\":\"DecisionTreeEnsemble\",\"FeatureFunctions\":[\"DominantFrequency\",\"MFCC\",\"MFCC\",\"MFE\",\"MFE\",\"MFE\"]}]}"};

int32_t recognition_model_string_json_len = sizeof(recognition_model_string_json);

#endif /* __MODEL_JSON_H__ */
