#include "bvtext_c.h"
#include "bvtext.h"

#include <cstring>

int bvtext_load(const char *xml_path)
{
    BvText::instance().createXML(xml_path);
    return 0;
}

int bvtext_get_value(const char *node_path, char *buf, int buf_size)
{
    if (!buf || buf_size <= 0)
        return -1;

    std::string val = BvText::instance().getValue(node_path);

    std::strncpy(buf, val.c_str(), static_cast<size_t>(buf_size) - 1);
    buf[buf_size - 1] = '\0';
    return 0;
}
