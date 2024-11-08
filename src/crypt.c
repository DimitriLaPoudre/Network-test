void encrypt(unsigned char str[256],unsigned  char key[7], unsigned char iv[256])
{
    unsigned char iv_tmp[256];

    {
        unsigned char hash = 0;

        for (int i = 0; i < 256; i++) {
            hash ^= (hash << 5) + (hash >> 2) + key[i % 7];
            iv_tmp[i] = iv[i] ^ hash;
        }
    }
    {
        for (int i = 0; i < 256; i++)
            str[i] ^= iv_tmp[i];
    }
    {
        int tmp = 0;

        for (int i = 0; i < 256; i++) {
            tmp = str[iv_tmp[i]];
            str[iv_tmp[i]] = str[i];
            str[i] = tmp;
        }
    }
    {
        for (int i = 0; i < 256; i++) {
            if (i % 2)
                str[i] += iv_tmp[i];
            else
                str[i] -= iv_tmp[i];
        }
    }
    {
        char tmp = 0;

        for (int i = 0; i < 256; i++) {
            tmp = str[iv_tmp[i]];
            str[iv_tmp[i]] = str[i];
            str[i] = tmp;
        }
    }
}

void decrypt(char str[256], char *key, char iv[256])
{
    unsigned char iv_tmp[256];

    {
        unsigned char hash = 0;

        for (int i = 0; i < 256; i++) {
            hash ^= (hash << 5) + (hash >> 2) + key[i % 7];
            iv_tmp[i] = iv[i] ^ hash;
        }
    }
    {
        int tmp = 0;

        for (int i = 256 - 1; i >= 0; i--) {
            tmp = str[iv_tmp[i]];
            str[iv_tmp[i]] = str[i];
            str[i] = tmp;
        }
    }
    {
        for (int i = 0; i < 256; i++) {
            if (i % 2)
                str[i] -= iv_tmp[i];
            else
                str[i] += iv_tmp[i];
        }
    }
    {
        int tmp = 0;

        for (int i = 256 - 1; i >= 0; i--) {
            tmp = str[iv_tmp[i]];
            str[iv_tmp[i]] = str[i];
            str[i] = tmp;
        }
    }
    {
        for (int i = 0; i < 256; i++)
            str[i] ^= iv_tmp[i];
    }
}
