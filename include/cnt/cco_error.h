#ifndef CNT_CCO_ERROR_H
#define CNT_CCO_ERROR_H

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        CCO_OK = 0,
        CCO_ERR_INVALID_ARG = -1,
        CCO_ERR_OUT_OF_RANGE = -2,
        CCO_ERR_TYPE_MISMATCH = -3,
        CCO_ERR_NOT_FOUND = -4,
        CCO_ERR_FORBIDDEN = -5,
        CCO_ERR_NO_MEMORY = -6,
        CCO_ERR_PARSE = -7,
        CCO_ERR_IO = -8
    } cco_error_t;

#ifdef __cplusplus
}
#endif

#endif /* CNT_CCO_ERROR_H */
