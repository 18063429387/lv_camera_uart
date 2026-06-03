#ifndef BVTEXT_C_H
#define BVTEXT_C_H

#ifdef __cplusplus
extern "C" {
#endif

// 加载XML文件，xml_path文件名
int bvtext_load(const char *xml_path);

// 按节点路径获取文本值 node_path  节点路径，如 "system/language-1"
int bvtext_get_value(const char *node_path, char *buf, int buf_size);

#ifdef __cplusplus
}
#endif

#endif
