
#ifdef FILHANTERING_CPP
#define FILHANTERING_CPP

#include <iostream>
#include <stdarg.h>

int errlog(const char *format, ...);

#define CHAR_ALLOC 257

struct dataStr
{
	int length;
	//	char data[257];
	char data[CHAR_ALLOC];
};

#endif // FILHANTERING_CPP

extern string resultPath; 

#include "pch.h"

const int MAX_nOBJ = 100;

int read_char(FILE *FilPek)
{     int c;
	  if (FilPek == NULL){
		  fprintf(stdout, "Error: Ovantat filslut upptackt\n");
		  errlog("Error: Ovantat filslut upptackt\n");
		  return EOF;
	  }
	  else{
	      c = fgetc(FilPek);
		  if (ferror(FilPek))
			  fprintf(stdout, "read error on ''%s'' - ''%s''", FilPek, strerror(errno));
		  if (feof(FilPek)) c = EOF;
	  }
      return c;
}

void append_char(int c, dataStr *data)
{
	int i;
	if (data->length >= 256){
		fprintf(stdout, "VARNING: datanamn mer an 256 tecken, resterande trunkeras bort\nNamn:");
		for(i=0;i<256;i++)
			fprintf(stdout, "%c",data->data[i]);
		fprintf(stdout, "\n");
	}
	data->data[data->length] = (char)c;
	(data->length)++;
}

int get_next_data_ej_rad(FILE *FilPek, dataStr *data)
{
	FILE *FilError;
	int c;
	data->length = 0;
	c = read_char(FilPek);
	while (c == ' ' || c == ';')
		c = read_char(FilPek);
	for (;;){
		if (c == EOF)
			return 1;
		else if (c == ';'){
			data->data[data->length] = '\0';
			break;
		}
		else if (c == '\n'){
			if (data->length > 0){
				errlog("ERROR: rad avslutas ej med ;\n");
			}
			return -1;
		}
		append_char(c, data);
		c = read_char(FilPek);
	}
	while (data->data[data->length - 1] == ' ' && data->length >= 2){
		data->data[data->length - 1] = '\0';
		(data->length)--;
	}
	return 0;
}

int get_next_data_ej_rad3(FILE *FilPek, dataStr *data)
{
	FILE *FilError;
	int c, returnVal = 0;
	data->length = 0;
	c = read_char(FilPek);
	while(c == ' ' || c == ';')
		c = read_char(FilPek);
	for(;;){
		if (c == EOF)
			return 1;
		else if (c == ';'){
			data->data[data->length] = '\0';
			break;
		}
		else if(c == '\n'){
			if (data->length > 0){
				data->data[data->length] = '\0';
				returnVal = -1;
				break;
			}
			return -1;
		}
		append_char(c, data);
		c = read_char(FilPek);		
	}
	while (data->data[data->length-1] == ' ' && data->length >=2){
		data->data[data->length-1] = '\0';
		(data->length)--;
	}
	return returnVal;
}


int get_next_data_ej_rad_msl(FILE *FilPek, dataStr *data, int *RadSlut)
{
	int c;
	data->length = 0;
	c = read_char(FilPek);
	while (c == ' ' || c == ';')
		c = read_char(FilPek);
	for (;;){
		if (c == EOF){
			*RadSlut = -1;
			return 1;
		}
		else if (c == ' ' || c == '\n'){
			data->data[data->length] = '\0';
			if (c == ' ')
				*RadSlut = 0;
			else
				*RadSlut = 1;
			break;
		}
		append_char(c, data);
		c = read_char(FilPek);
	}
	while (data->data[data->length - 1] == ' ' && data->length >= 2){
		data->data[data->length - 1] = '\0';
		(data->length)--;
	}
	return 0;
}

int get_next_data_ej_rad_komma(FILE *FilPek, dataStr *data, int *RadSlut)
{
	int c;
	data->length = 0;
	c = read_char(FilPek);
	while(c == ',')
		c = read_char(FilPek);
	for(;;){
		if (c == EOF){
			*RadSlut = -1;
			return 1;
		}
		else if (c == ',' || c == '\n'){
			data->data[data->length] = '\0';
			if(c == ',')
				*RadSlut = 0;
			else
				*RadSlut = 1;
			break;
		}
		append_char(c, data);
		c = read_char(FilPek);		
	}
	while (data->data[data->length-1] == ' ' && data->length >=2){
		data->data[data->length-1] = '\0';
		(data->length)--;
	}
	return 0;
}



int get_next_data_ej_rad_mslsemkol(FILE *FilPek, dataStr *data, int *RadSlut)
{
	int c;
	data->length = 0;
	c = read_char(FilPek);
	while(c == ' ' || c == ';')
		c = read_char(FilPek);
	for(;;){
		if (c == EOF){
			*RadSlut = -1;
			return 1;
		}
		else if (c == ' ' || c == '\n' || c == ';'){
			data->data[data->length] = '\0';
			if(c == ' ' || c == ';')
				*RadSlut = 0;
			else
				*RadSlut = 1;
			break;
		}
		append_char(c, data);
		c = read_char(FilPek);		
	}
	while (data->data[data->length-1] == ' ' && data->length >=2){
		data->data[data->length-1] = '\0';
		(data->length)--;
	}
	return 0;
}


int get_next_data_ej_rad_semkol(FILE *FilPek, dataStr *data, int *RadSlut)
{
	int c;
	data->length = 0;
	c = read_char(FilPek);
	while(c == ';')
		c = read_char(FilPek);
	for(;;){
		if (c == EOF){
			*RadSlut = -1;
			return 1;
		}
		else if (c == '\n' || c == ';'){
			data->data[data->length] = '\0';
			if(c == ';')
				*RadSlut = 0;
			else
				*RadSlut = 1;
			break;
		}
		append_char(c, data);
		c = read_char(FilPek);		
	}
	return 0;
}


int get_next_data_ej_rad2(FILE *FilPek, dataStr *data)
{
	FILE *FilError;
	int c;
	int Fnuttar;
	data->length = 0;
	c = read_char(FilPek);
//	while(c == ' ' || c == ';')
	while(c == ' ')
		c = read_char(FilPek);
	if(c == '"')
		Fnuttar = 1;
	else
		Fnuttar = 0;
	for(;;){
		if (c == EOF)
			return 1;
		else if (c == ';'){
			data->data[data->length] = '\0';
			break;
		}
		else if(c == '\n'){
			if(data->length > 0){
				errlog ("ERROR: rad avslutas ej med ;\n");
			}
			return -1;
		}
		if(Fnuttar == 0 || c != '"')
			append_char(c, data);
		c = read_char(FilPek);		
	}
	while (data->data[data->length-1] == ' ' && data->length >=2){
		data->data[data->length-1] = '\0';
		(data->length)--;
	}
	return 0;
}

int get_data_objects_till_EOL_msl(char objects[][CHAR_ALLOC], dataStr *data, FILE *FilPek)
{
	int antal, avbrott, returnvarde, RadSlut = 0;

	avbrott = 0;
	antal = 0;
	for (;;){
		returnvarde = get_next_data_ej_rad_msl(FilPek, data, &RadSlut);
		if (returnvarde == 1){
			return -antal;
		}
		else{
			strcpy(objects[antal], data->data);
			antal++;
			if (RadSlut == 1)
				return antal;
		}
	}

}

int get_data_objects_till_EOL_komma(char objects[][CHAR_ALLOC], dataStr *data, FILE *FilPek)
{
	int antal, avbrott, returnvarde, RadSlut = 0;

	avbrott = 0;
	antal = 0;
	for(;;){
		returnvarde = get_next_data_ej_rad_komma(FilPek, data, &RadSlut);
		if(returnvarde == 1){
			return -antal;
		}
		else{
			strcpy(objects[antal], data->data);
			antal++;
			if(RadSlut == 1)
				return antal;
		}
	}

}


int get_data_objects_till_EOL_mslsemkol(char objects[][CHAR_ALLOC], dataStr *data, FILE *FilPek)
{
	int antal, avbrott, returnvarde, RadSlut = 0;

	avbrott = 0;
	antal = 0;
	for(;;){
		returnvarde = get_next_data_ej_rad_mslsemkol(FilPek, data, &RadSlut);
		if(returnvarde == 1){
			return -antal;
		}
		else{
			strcpy(objects[antal], data->data);
			antal++;
			if(RadSlut == 1){
				if(data->length == 0)
					return antal-1;
				else
					return antal;
			}
		}
	}

}

int get_data_objects_till_EOL_semkol(char objects[][CHAR_ALLOC], dataStr *data, FILE *FilPek)
{
	int antal, avbrott, returnvarde, RadSlut = 0;

	avbrott = 0;
	antal = 0;
	for(;;){
		returnvarde = get_next_data_ej_rad_semkol(FilPek, data, &RadSlut);
		if(returnvarde == 1){
			return -antal;
		}
		else{
			strcpy(objects[antal], data->data);
			antal++;
			if(RadSlut == 1){
				if(data->length == 0)
					return antal-1;
				else
					return antal;
			}
		}
	}

}

int get_data_objects_till_EOL(char objects[][CHAR_ALLOC], dataStr *data, FILE *FilPek)
{
	int antal, avbrott, returnvarde;

	avbrott = 0;
	antal = 0;
	for (;;){
		returnvarde = get_next_data_ej_rad(FilPek, data);
		if (returnvarde == 1){
			return -antal;
		}
		else if (returnvarde == -1)
			return antal;
		else{
			strcpy(objects[antal], data->data);
			antal++;
		}
	}

}

int get_data_objects_till_EOL2(char objects[][CHAR_ALLOC], dataStr *data, FILE *FilPek)
{
	int antal, avbrott, returnvarde;

	avbrott = 0;
	antal = 0;
	for(;;){
		returnvarde = get_next_data_ej_rad2(FilPek, data);
		if(returnvarde == 1){
			return -antal;
		}
		else if (returnvarde == -1){
			if (data->length > 0){
				strcpy(objects[antal], data->data);
				antal++;
			}
			return antal;
		}
		else{
			strcpy(objects[antal], data->data);
			antal++;
		}
	}

}

int get_data_objects_till_EOL3(char objects[][CHAR_ALLOC], dataStr *data, FILE *FilPek)
{
	int antal, avbrott, returnvarde;

	avbrott = 0;
	antal = 0;
	for (;;){
		returnvarde = get_next_data_ej_rad3(FilPek, data);
		if (returnvarde == 1){
			return -antal;
		}
		else if (returnvarde == -1){
			if (data->length > 0){
				strcpy(objects[antal], data->data);
				antal++;
			}
			return antal;
		}
		else{
			strcpy(objects[antal], data->data);
			antal++;
		}
	}

}

int get_next_data_ej_rad10(FILE *FilPek, dataStr *data, int Separator)
{
	int c;
	data->length = 0;
	//	if ( EOF ( FilPek ) )
	//		return 1;
	c = read_char(FilPek);
	if (c == 239){
		c = read_char(FilPek);
		c = read_char(FilPek);
		c = read_char(FilPek);
	}
	while (c == Separator)
		c = read_char(FilPek);
	for (;;){
		if (c == EOF){
			if (data->length > 0)
				data->data[data->length] = '\0';
			return 1;
		}
		else if (c == Separator){
			data->data[data->length] = '\0';
			break;
		}
		else if (c == '\n'){
			data->data[data->length] = '\0';
			return -1;
		}
		append_char(c, data);
		c = read_char(FilPek);
	}
	while (data->data[data->length - 1] == ' ' && data->length >= 2){
		data->data[data->length - 1] = '\0';
		(data->length)--;
	}
	return 0;
}

int get_data_objects_till_EOL_or_maxAntal(char objects[][CHAR_ALLOC], dataStr *data, int Separator, FILE *FilPek, int maxAntal)
{
	int antal, avbrott, returnvarde;

	avbrott = 0;
	for (antal = 0; antal < maxAntal;){
		returnvarde = get_next_data_ej_rad10(FilPek, data, Separator);
		if (returnvarde == 1){
			if (data->length > 0){
				strcpy(objects[antal], data->data);
				antal++;
				if (antal >= MAX_nOBJ - 1)
					fprintf(stdout, "ERROR: for manga element i objektet, bryr mig ej om resten..., rad %d\n", __LINE__);
				return antal;
			}
			else
				return -antal;
		}
		else if (returnvarde <= -1 || antal >= MAX_nOBJ - 1){
			if (returnvarde == -1){
				strcpy(objects[antal], data->data);
				antal++;
			}
			if (antal >= MAX_nOBJ - 1)
				fprintf(stdout, "ERROR: for manga element i objektet, bryr mig ej om resten..., rad %d\n", __LINE__);
			return antal;
		}
		else{
			strcpy(objects[antal], data->data);
			antal++;
		}
	}

	return antal;

}

int get_data_objects_till_EOL_orMaxAlloc(char objects[][CHAR_ALLOC], dataStr *data, FILE *FilPek,
																				 int nMaxAlloc, int *RadKlar)
{
	int antal, avbrott, returnvarde;

	avbrott = 0;
	antal = 0;
	for(;;){
		if (antal >= 102)
			antal = antal;
		returnvarde = get_next_data_ej_rad(FilPek, data);
		if(returnvarde == 1){
			*RadKlar = 2;
			return antal;
		}
		else if(returnvarde == -1){
			*RadKlar = 1;
			return antal;
		}else{
			strcpy(objects[antal], data->data);
			antal++;
			if(antal >= nMaxAlloc){
				*RadKlar = 0;
				return antal;
			}
		}
	}

}

char *str_alloc_cpy(const char *data)
{
	char *dataAdd;
	dataAdd = (char*)malloc((strlen(data) + 1)*sizeof(char));
	if (dataAdd == NULL){
		fprintf(stdout, "out of memory at line %d\n", __LINE__);
	}
	strcpy(dataAdd, data);
	return dataAdd;
}

char *addPathToName(char *data0, char *path)
{
	char *dataAdd;
	int langd;
	langd = strlen(data0) + strlen(path) + 2;
	dataAdd = (char*)malloc(langd * sizeof(char));
	if (dataAdd == NULL){
		fprintf(stdout, "out of memory at line %d\n", __LINE__);
	}
	strcpy(dataAdd, path);
	if (dataAdd[strlen(dataAdd) - 1] != '\\')
		strcat(dataAdd, "\\");
	strcat(dataAdd, data0);
	free(data0);
	return dataAdd;
}

double char_to_double(char* object)
{
	double varde, faktor;
	int i, komma, negativ, startPos, potens10 = 0, valPotens;
	int negativPotens;
	komma = 0;
	varde = 0;
	faktor = 0.1;
	if (object[0] == '-') {
		negativ = 1;
		startPos = 1;
	}
	else {
		negativ = 0;
		startPos = 0;
	}
	for (i = startPos;; i++) {
		if (object[i] != '.' && object[i] != ',' && object[i] != '\0' && potens10 == 0 && object[i] != 'e' && object[i] != 'E') {
			if (komma == 0)
				varde = varde * 10 + (object[i] - '0');
			else {
				varde = varde + (object[i] - '0') * faktor;
				faktor = faktor / 10;
			}
		}
		else if (object[i] == '.' || object[i] == ',')
			komma = 1;
		else if (object[i] == '\0')
			break;
		else {
			if (potens10 == 1) {
				if (object[i] == '-')
					negativPotens = 1;
				else if (object[i] == '+')
					negativPotens = 0;
				else
					valPotens = valPotens * 10 + (object[i] - '0');
			}
			else {
				valPotens = 0;
				potens10 = 1;
				negativPotens = 0;
			}
		}
	}

	if (potens10 == 1) {
		if (negativPotens == 0)
			varde *= pow((double)10, valPotens);
		else {
			if (valPotens > 0)
				varde /= (pow((double)10, valPotens));
		}
	}
	if (negativ == 1)
		varde = -varde;

	return varde;
}

double char_to_doubleConst(const char *object)
{
	double varde, faktor;
	int i, komma, negativ, startPos, potens10 = 0, valPotens;
	int negativPotens;
	komma = 0;
	varde = 0;
	faktor = 0.1;
	if(object[0] == '-'){
		negativ = 1;
		startPos = 1;
	}else{
		negativ = 0;
		startPos = 0;
	}
	for(i=startPos;;i++){
		if(object[i] != '.' && object[i] != ',' && object[i] != '\0' && potens10 == 0 && object[i] != 'e' && object[i] != 'E'){
			if(komma == 0)
				varde = varde*10 + (object[i] - '0');
			else{
				varde = varde + (object[i] - '0')*faktor;
				faktor=faktor/10;
			}
		}else if(object[i] == '.' || object[i] == ',')
			komma = 1;
		else if (object[i] == '\0')
			break;
		else{
			if (potens10 == 1){
				if (object[i] == '-')
					negativPotens = 1;
				else if (object[i] == '+')
					negativPotens = 0;
				else
					valPotens = valPotens*10 + (object[i] - '0');
			}else{
				valPotens = 0;
				potens10 = 1;
				negativPotens = 0;
			}
		}
	}

	if (potens10 == 1){
		if (negativPotens == 0)
			varde *= pow ((double)10, valPotens);
		else{
			if (valPotens > 0)
				varde /= (pow ((double)10, valPotens));
		}
	}
	if(negativ == 1)
		varde = -varde;

	return varde;
}

long long char_to_longlong(char *object)
{
	double varde, faktor;
	FILE *FilError;
	long long varde2;
	int i, komma, negativ, startPos, potens10 = 0, valPotens;
	int negativPotens;
	komma = 0;
	varde = 0;
	faktor = 0.1;
	if(object[0] == '-'){
		negativ = 1;
		startPos = 1;
	}else{
		negativ = 0;
		startPos = 0;
	}
	for(i=startPos;;i++){
		if(object[i] != '.' && object[i] != ',' && object[i] != '\0' && potens10 == 0 && object[i] != 'e' && object[i] != 'E'){
			if(komma == 0)
				varde = varde*10 + (object[i] - '0');
			else{
				varde = varde + (object[i] - '0')*faktor;
				faktor=faktor/10;
			}
		}else if(object[i] == '.' || object[i] == ','){
			komma = 1;
			errlog ("ERROR: kommatecken i heltal\n");
		}else if (object[i] == '\0')
			break;
		else{
			if (potens10 == 1){
				if (object[i] == '-')
					negativPotens = 1;
				else if (object[i] == '+')
					negativPotens = 0;
				else
					valPotens = valPotens*10 + (object[i] - '0');
			}else{
				valPotens = 0;
				potens10 = 1;
				negativPotens = 0;
			}
		}
	}

	if (potens10 == 1){
		if (negativPotens == 0)
			varde *= pow ((double)10, valPotens);
		else{
			if (valPotens > 0)
				varde /= (pow ((double)10, valPotens));
		}
	}
	if(negativ == 1)
		varde = -varde;

	varde2 = (long long) varde;
	return varde2;
}
long long char_to_longlong2(char *object)
{
	FILE *FilError;
	long long varde;
	int i, negativ, startPos;
	varde = 0;
	if(object[0] == '-'){
		negativ = 1;
		startPos = 1;
	}else{
		negativ = 0;
		startPos = 0;
	}
	for(i=startPos;;i++){
		if(object[i] == '.' || object[i] == ','){
			errlog ("ERROR: kommatecken i heltal\n");
		}else if (object[i] == '\0')
			break;
		else
			varde = varde*10 + (object[i] - '0');
	}

	if(negativ == 1)
		varde = -varde;
	return varde;
}


int char_to_int(char *object)
{
	FILE *FilError;
	int varde;
	int i, negativ, startPos;
	varde = 0;
	if(object[0] == '-'){
		negativ = 1;
		startPos = 1;
	}else{
		negativ = 0;
		startPos = 0;
	}
	for(i=startPos;;i++){
		if(object[i] == '.' || object[i] == ','){
			errlog ("ERROR: kommatecken i heltal\n");
		}else if (object[i] == '\0')
			break;
		else
			varde = varde*10 + (object[i] - '0');
	}

	if(negativ == 1)
		varde = -varde;
	return varde;
}


int char_to_intSpec(char *object)
{
	FILE *FilError;
	int varde, Faktor;
	int i, i1;
	varde = 0;

	for(i=0;;i++){
		if (object[i] == '\0')
			break;
	}
	for(;i >= 0; i--){
		if(object[i] >= '0' && object[i] <= '9'){
			i++;
			break;
		}
	}
	if(i < 5){
		errlog ("ERROR: felaktigt ruttnamn: %s\n", object);
	}
	Faktor = 1;
	for(i1 = 1; i1 < 5; i1++){
		varde += (object[i-i1] - '0')*Faktor;
		Faktor *= 10;
	}
	return varde;
}

int reset_errlog()
{
	FILE *log;
	string namn = resultPath + "/logfile.txt";
	log = fopen(namn.c_str(), "w");
	fclose(log);
	return 0;
}

int errlog0(const char* format, ...)
{
	va_list args;


	FILE* log;

	log = fopen("logfile.txt", "a+");

	if (log == NULL)
		return -1;

	va_start(args, format);
	vfprintf(log, format, args);
	va_end(args);
	fclose(log);

	return 0;
}

int errlog (const char *format, ...)
{
  va_list args;
  string namn;

  FILE *log;
  namn = resultPath + "/logfile.txt";

  log = fopen (namn.c_str(), "a+");

  if (log == NULL)
    return -1;

  va_start (args, format);
  vfprintf (log, format, args);
  va_end (args);
  fclose (log);

  return 0;
}


int write_copyAtoB(char *filnamnUt, char *filExt, char *filenamnIn, char *mode)
{
	char fil2[256];
	char strang[100000];
	FILE *FilIn, *FilUt;


	strcpy(fil2, filnamnUt);
	strcat(fil2, ".");
	strcat(fil2, filExt);
	FilIn = fopen(filenamnIn, "r");
	if (FilIn != NULL) {
		FilUt = fopen(fil2, mode);
		for (; fgets(strang, 100000, FilIn) != NULL;) {
			fprintf(FilUt, "%s", strang);
		}
		fclose(FilIn);
		fclose(FilUt);
	}
	else
		errlog("ERROR! Skulle spara fil till resDir men fanns ej: %s\n", filenamnIn);

	return 0;
}



int test2(int a) {
	return 2 * a;
}
