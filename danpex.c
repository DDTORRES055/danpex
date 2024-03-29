#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "danpex.h"
#include "sha256.h"

int verify_strong_password(unsigned char *p)
{

  char *string1 = (char *)p;
  int hasUpper = 0;
  int hasLower = 0;
  int hasDigit = 0;
  int hasEspecial = 0;

  for (int i = 0; i < strlen(string1); ++i)
  {
    if (islower(string1[i]))
      hasLower = 1;

    if (isupper(string1[i]))
      hasUpper = 1;

    if (isdigit(string1[i]))
      hasDigit = 1;

    if (!isdigit(string1[i]) && !isupper(string1[i]) && !islower(string1[i]))
      hasEspecial = 1;
  }
  if (strlen(string1) > 8 && hasLower && hasUpper && hasDigit && hasEspecial)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

void lfsr128_set_password(lfsr128_t *l, unsigned char *p)
{
  BYTE buf[SHA256_BLOCK_SIZE];
  SHA256_CTX ctx;
  uint64_t lfsr_h;
  uint64_t lfsr_l;

  sha256_init(&ctx);
  sha256_update(&ctx, p, strlen((char *)p));
  sha256_final(&ctx, buf);
  memcpy(&lfsr_h, buf, sizeof(uint64_t));
  memcpy(&lfsr_l, buf + sizeof(uint64_t), sizeof(uint64_t));
  lfsr128_init(l, lfsr_h, lfsr_l);
}

void lfsr128x3_set_from_captain(lfsr128_t *captain, lfsr128x3_t *l)
{
  l->lfsr[0].lfsr_h = lfsr128_shiftn(captain, 64);
  l->lfsr[0].lfsr_l = lfsr128_shiftn(captain, 64);
  l->lfsr[1].lfsr_h = lfsr128_shiftn(captain, 64);
  l->lfsr[1].lfsr_l = lfsr128_shiftn(captain, 64);
  l->lfsr[2].lfsr_h = lfsr128_shiftn(captain, 64);
  l->lfsr[2].lfsr_l = lfsr128_shiftn(captain, 64);
}

void lfsr128x3_set_password(lfsr128x3_t *l, unsigned char *p)
{
  lfsr128_t lfsr128_captain;

  lfsr128_set_password(&lfsr128_captain, p);
  lfsr128x3_set_from_captain(&lfsr128_captain, l);
}

void lfsr128x3_set_init_state(lfsr128x3_t *l, lfsr128_t *initState)
{
  l->lfsr[0].lfsr_h = initState->lfsr_h;
  l->lfsr[0].lfsr_l = initState->lfsr_l;
  l->lfsr[1].lfsr_h = initState->lfsr_h;
  l->lfsr[1].lfsr_l = initState->lfsr_l;
  l->lfsr[2].lfsr_h = initState->lfsr_h;
  l->lfsr[2].lfsr_l = initState->lfsr_l;
}

void lfsr128x3_set_cap_state(lfsr128x3_t *l, lfsr128_t *initState)
{
  lfsr128_t lfsr128_captain;

  lfsr128_init(&lfsr128_captain, initState->lfsr_h, initState->lfsr_l);
  lfsr128x3_set_from_captain(&lfsr128_captain, l);
}

void lfsr128_init(lfsr128_t *l, uint64_t lfsr_h, uint64_t lfsr_l)
{
  l->lfsr_h = lfsr_h;
  l->lfsr_l = lfsr_l;
}

uint64_t lfsr128_shift(lfsr128_t *l)
{
  uint64_t bit, bit_h, r;
  r = l->lfsr_l & 1;
  bit = ((l->lfsr_l >> 0) ^ (l->lfsr_l >> 1) ^ (l->lfsr_l >> 2) ^
         (l->lfsr_l >> 7)) &
        1;
  bit_h = l->lfsr_h & 1;
  l->lfsr_l = (l->lfsr_l >> 1) | (bit_h << 63);
  l->lfsr_h = (l->lfsr_h >> 1) | (bit << 63);

  return r;
}

/* 
 * Return the carry bit, not the shited out bit
 */
uint64_t lfsr128_shift_return_carry(lfsr128_t *l)
{
  uint64_t bit, bit_h;
  bit = ((l->lfsr_l >> 0) ^ (l->lfsr_l >> 1) ^ (l->lfsr_l >> 2) ^
         (l->lfsr_l >> 7)) &
        1;
  bit_h = l->lfsr_h & 1;
  l->lfsr_l = (l->lfsr_l >> 1) | (bit_h << 63);
  l->lfsr_h = (l->lfsr_h >> 1) | (bit << 63);

  return bit;
}

uint64_t lfsr128_shiftn(lfsr128_t *l, uint8_t n)
{
  uint64_t r = 0;
  int i;
  r = lfsr128_shift(l);
  for (i = 0; i < n - 1; i++)
  {
    r = r << 1;
    r = r | lfsr128_shift(l);
  }

  return r;
}

uint64_t lfsr128_shift_with_mult_dec(lfsr128x3_t *l)
{
  uint64_t r0, r1, r2;

  r0 = lfsr128_shift(&l->lfsr[0]);
  r1 = lfsr128_shift(&l->lfsr[1]);
  r2 = lfsr128_shift_return_carry(&l->lfsr[2]);

  if (r2 == 1)
  {
    /* Decimate r0 by 1 bit, r2 by 2 bits*/
    r0 = lfsr128_shift(&l->lfsr[0]);
    r1 = lfsr128_shift(&l->lfsr[1]);
    r1 = lfsr128_shift(&l->lfsr[1]);
  }

  return r0 ^ r1;
}

uint64_t lfsr128_shiftn_with_mult_dec(lfsr128x3_t *l, uint8_t n)
{
  uint64_t r = 0;
  int i;

  r = lfsr128_shift_with_mult_dec(l);
  for (i = 0; i < n - 1; i++)
  {
    r = r << 1;
    r = r | lfsr128_shift_with_mult_dec(l);
  }

  return r;
}

void code_buffer(uint8_t *b, lfsr128x3_t *l, int sz)
{
  for (int i = 0; i < sz; i++)
  {
    b[i] = b[i] ^ (uint8_t)lfsr128_shiftn_with_mult_dec(l, 8);
  }
}

void do_print_random_numbers(lfsr128x3_t *l, int sz)
{
  uint64_t ln = 0;

  do
  {
    ln++;
    printf("%03d\n", (uint8_t)lfsr128_shiftn_with_mult_dec(l, 8));
  } while (ln < sz);
}

void usage()
{
  printf(
      "danpex"
      "Uso: danpex [-p contraseña] [-n desplazamiento] <entrada> <salida>\n"
      "Opciones: \n"
      "\t-p - Contraseña, por favor use una contraseña de mas de 8 caracteres que contenga al menos mayusculas, minusculas, numeros y caracteres espeaciales\n"
      "\t-n - Un numero aleatorio para el desplazamiento\n"
      "\t-r - Imprimir los numeros aleatorios\n"
      "Argumentos: \n"
      "\tentrada - Ruta del archivo de entrada \n"
      "\tsalida - Ruta del archivo de salida \n");
}

int main(int argc, char *argv[])
{

  int opt;
  int offset = -1;
  int verbose = 0;
  int print_random_numbers = 0;
  unsigned char *password = NULL;
  char *input_fn = NULL;
  char *output_fn = NULL;
  lfsr128x3_t lfsr;
  int how_many_rand_nums = 1000000;
  int use_init_state = 0;
  int init_state_captain = 0;
  lfsr128_t init_state = {0, 0};

  while ((opt = getopt(argc, argv, "rl:n:p:hv1:2:c?")) != -1)
  {
    switch (opt)
    {
    case 'v':
      verbose = 1;
      break;
    case 'r':
      print_random_numbers = 1;
      break;
    case 'l':
      how_many_rand_nums = atoi(optarg);
      break;
    case 'n':
      offset = atoi(optarg);
      break;
    case 'p':
      password = malloc(strlen(optarg) + 1);
      strcpy((char *)password, optarg);
      break;
    case 'h':
      usage();
      exit(EXIT_FAILURE);
      break;
    case '1':
      use_init_state = 1;
      init_state.lfsr_l = strtoull(optarg, NULL, 16);
      break;
    case '2':
      use_init_state = 1;
      init_state.lfsr_h = strtoull(optarg, NULL, 16);
      break;
    case 'c':
      init_state_captain = 1;
      break;
    default: /* '?' */
      usage();
      exit(EXIT_FAILURE);
    }
  }

  if (password == NULL && !use_init_state)
  {
    fprintf(stderr, "Se debe proporcionar una contraseña.\n\n");
    usage();
    exit(EXIT_FAILURE);
  }
  else if (!verify_strong_password(password))
  {
    printf("Contraseña insegura, por favor use una contraseña de mas de 8 caracteres que contenga al menos mayusculas, minusculas, numeros y caracteres espeaciales\n\n");
    exit(EXIT_FAILURE);
  }

  if (print_random_numbers == 0)
  {
    if (optind >= argc)
    {
      fprintf(stderr, "Se esperaban dos argumentos despues de las opciones.\n\n");
      usage();
      exit(EXIT_FAILURE);
    }

    if (optind + 1 >= argc)
    {
      fprintf(stderr, "Se espera otro argumento.\n\n");
      usage();
      exit(EXIT_FAILURE);
    }
    input_fn = malloc(strlen(argv[optind]) + 1);
    strcpy(input_fn, argv[optind]);
    output_fn = malloc(strlen(argv[optind + 1]) + 1);
    strcpy(output_fn, argv[optind + 1]);
  }

  if (verbose)
  {
    if (offset > 0)
    {
      printf("Desplazamiento (-n): %d\n", offset);
    }

    if (password != NULL)
    {
      printf("Contraseña: %s\n", password);
    }
    if (use_init_state)
    {
      printf(
          "Se ha forzado el estado inicial en %s: %lx,%lx\n",
          init_state_captain ? "captain" : "LFSRs",
          init_state.lfsr_h,
          init_state.lfsr_l);
    }
    if (print_random_numbers)
    {
      printf("Imprimiendo los numeros aleatorios...\n");
    }
    else
    {
      printf("Archivo de entrada: %s\n", input_fn);
      printf("Archivo de salia: %s\n", output_fn);
    }
  }

  if (use_init_state)
  {
    if (init_state_captain)
    {
      lfsr128x3_set_cap_state(&lfsr, &init_state);
    }
    else
    {
      lfsr128x3_set_init_state(&lfsr, &init_state);
    }
  }
  else
  {
    lfsr128x3_set_password(&lfsr, password);
  }

  if (offset > 0)
  {
    for (int i = 0; i < offset; i++)
    {
      lfsr128_shiftn_with_mult_dec(&lfsr, 8);
    }
  }

  if (print_random_numbers)
  {
    do_print_random_numbers(&lfsr, how_many_rand_nums);
    return EXIT_SUCCESS;
  }

  FILE *fp_in = fopen(input_fn, "rb");
  if (!fp_in)
  {
    perror("No es posible abrir el archivo de entrada, verifique los permisos y la ruta introducida");
    return EXIT_FAILURE;
  }

  FILE *fp_out = fopen(output_fn, "wb");
  if (!fp_out)
  {
    perror("No es posible escribir en el archivo de salida, verifique los permisos y la ruta introducida");
    return EXIT_FAILURE;
  }

  uint8_t buffer[BUFFER_SZ];
  size_t n;

  while ((n = fread(buffer, 1, BUFFER_SZ, fp_in)) != 0)
  {
    code_buffer(buffer, &lfsr, n);
    fwrite(buffer, 1, n, fp_out);
  }
  if (n > 0)
  {
    code_buffer(buffer, &lfsr, n);
    fwrite(buffer, 1, n, fp_out);
  }

  fclose(fp_in);
  fclose(fp_out);

  return EXIT_SUCCESS;
}
