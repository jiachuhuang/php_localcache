
#ifndef LOC_LOC_SERIALIZER_H
#define LOC_LOC_SERIALIZER_H

int loc_serializer_pack(zval *data, smart_str *sb);
zval* loc_serializer_pack_unpack(char *content, size_t len, zval *return_val);

#endif /* LOC_LOC_SERIALIZER_H */