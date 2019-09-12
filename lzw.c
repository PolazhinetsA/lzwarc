#include "lzw.h"
#include "htbl.h"
#include "bitio.h"

#define DICTSIZE 0x4000

enum { Prev, Suff };

typedef struct {
    int (*ent)[2];
    int   nent;
    void *htbl;
} dict_t;

dict_t *dict_new();
void dict_free(dict_t *);
void dict_add(dict_t *, int, int);
int dict_find(dict_t *, int, int);

void lzw_encode(FILE *in, FILE *out)
{
    dict_t *dict = dict_new();
    void *bout = bopen(out);

    for (int prev, next, ch = 0, nbits = 9, stop = 0x100; ch != EOF; )
    {
        for(next = -1;
            prev = next,
            EOF != (ch = fgetc(in)) &&
             -1 != (next = dict_find(dict, prev, ch)); );
        if (prev >= stop) {
            bput(bout, stop, nbits);
            ++nbits;
            stop <<= 1;
        }
        bput(bout, prev, nbits);
        dict_add(dict, prev, ungetc(ch, in));
    }
    bflush(bout);

    free(bout);
    dict_free(dict);
}

void lzw_decode(FILE *in, FILE *out)
{
    dict_t *dict = dict_new();
    void *bin = bopen(in);

    for(int prev = -1, suff, next, nbits = 9, stop = 0x100;
        -1 != bget(bin, &next, nbits);
        prev = next)
    {
        if (next == stop) {
            ++nbits;
            stop <<= 1;
            next = prev;
            continue;
        }

        int add = next == dict->nent;
        if (add) dict_add(dict, prev, suff);

        unsigned char buf[DICTSIZE];
        int pos = DICTSIZE, len = 0, curr = next;
        do buf[--pos] = dict->ent[curr][Suff], ++len;
            while(-1 != (curr = dict->ent[curr][Prev]));

        fwrite(buf+pos, 1, len, out);
        suff = buf[pos];
        if (!add && prev != -1)
            dict_add(dict, prev, suff);
    }

    free(bin);
    dict_free(dict);
}

int enthash(void *, int);
int entcmp(int *, int *);

dict_t *dict_new()
{
    dict_t *this = malloc(sizeof(dict_t));
    this->ent = calloc(DICTSIZE, 2 * sizeof(int));
    this->nent = 0;
    this->htbl = htbl_new(20011, enthash, entcmp);

    unsigned char ch = 0;
    do dict_add(this, -1, ch); while (++ch);

    return this;
}

void dict_free(dict_t *this)
{
    htbl_free(this->htbl, NULL);
    free(this->ent);
    free(this);
}

void dict_add(dict_t *this, int prev, int suff)
{
    if (this->nent == DICTSIZE) return;

    this->ent[this->nent][Prev] = prev;
    this->ent[this->nent][Suff] = suff;
    htbl_add(this->htbl, this->ent[this->nent]);

    ++this->nent;
}

int dict_find(dict_t *this, int prev, int suff)
{
    int ent[2];
    ent[Prev] = prev;
    ent[Suff] = suff;

    int (*pent)[2] = htbl_find(this->htbl, ent);
    return pent ? pent - this->ent : -1;
}

int enthash(void *ent, int rng)
{
    return memhash(ent, 2 * sizeof(int), rng);
}

int entcmp(int *ent1, int *ent2)
{
    return ent1[Prev] - ent2[Prev]
           ? ent1[Prev] - ent2[Prev]
           : ent1[Suff] - ent2[Suff];
}