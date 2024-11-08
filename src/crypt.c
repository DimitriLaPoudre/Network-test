char *encrypt(char *str, char key[7])
{
    unsigned char minikey[8];

    for (int i = 0; i < 7; i++)
        minikey[i] = key[i];
    {
        minikey[7] = 2;
        for (int i = 0; str[i]; i++)
            str[i] ^= minikey[i % 8];
    }
    {
        int len = 0;
        int tmp = 0;

        for (; str[len]; len++);
        minikey[7] = 13;
        for (int i = 0; i < len; i++) {
            tmp = str[minikey[i % 8] % len];
            str[minikey[i % 8] % len] = str[i];
            str[i] = tmp;
        }
    }
    {
        minikey[7] = 2013 >> 8;
        for (int i = 0; str[i]; i++) {
            if (i % 2)
                str[i] += minikey[i % 8];
            else
                str[i] -= minikey[i % 8];
        }
    }
    {
        int len = 0;
        int tmp = 0;

        for (; str[len]; len++);
        minikey[7] = 2013 & 0xFF;
        for (int i = 0; i < len; i++) {
            tmp = str[minikey[i % 8] % len];
            str[minikey[i % 8] % len] = str[i];
            str[i] = tmp;
        }
    }
    return str;
}

char *decrypt(char *str, char *key)
{
    unsigned char minikey[8];

    for (int i = 0; i < 7; i++)
        minikey[i] = key[i];
    {
        int len = 0;
        int tmp = 0;

        for (; str[len]; len++);
        minikey[7] = 2013 & 0xFF;
        for (int i = len - 1; i >= 0; i--) {
            tmp = str[minikey[i % 8] % len];
            str[minikey[i % 8] % len] = str[i];
            str[i] = tmp;
        }
    }
    {
        minikey[7] = 2013 >> 8;
        for (int i = 0; str[i]; i++) {
            if (i % 2)
                str[i] -= minikey[i % 8];
            else
                str[i] += minikey[i % 8];
        }
    }
    {
        int len = 0;
        int tmp = 0;

        for (; str[len]; len++);
        minikey[7] = 13;
        for (int i = len - 1; i >= 0; i--) {
            tmp = str[minikey[i % 8] % len];
            str[minikey[i % 8] % len] = str[i];
            str[i] = tmp;
        }
    }
    {
        minikey[7] = 2;
        for (int i = 0; str[i]; i++)
            str[i] ^= minikey[i % 8];
    }
    return str;
}
