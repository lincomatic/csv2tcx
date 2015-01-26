/*
 * copyright (c) 2015 Sam C. Lin
 * convert GPS Master CSV file to TCX
 */
#include <stdio.h>
#include <string.h>

#define MAX_PTS 32768

class Datum {
  char *buf;
public:
  char *time;
  char *satcnt;
  char *hr;
  char *speed;
  char *lon;
  char *lat;
  char *alt;
  char *heading;
  char *dist;
  Datum() { buf = NULL; }
  ~Datum() { if (buf) delete [] buf; }
  int Set(char *line);
};

int Datum::Set(char *line)
{
  int slen = strlen(line);
  buf = new char[slen+1];
  if (buf) {
    strcpy(buf,line);
    buf[slen] = 0;

    char *s = buf;
    s = strchr(s,',');
    *(s++) = 0;
    this->time = s;
    *(strchr(this->time,' ')) = 'T';
    s = strchr(s,',');
    *(s++) = 0;
    this->satcnt = s;
    s = strchr(s,',');
    *(s++) = 0;
    this->hr = s;
    s = strchr(s,',');
    *(s++) = 0;
    this->speed = s;
    s = strchr(s,',');
    *(s++) = 0;
    this->lon = s;
    s = strchr(s,',');
    *(s++) = 0;
    this->lat = s;
    s = strchr(s,',');
    *(s++) = 0;
    this->alt = s;
    s = strchr(s,',');
    *(s++) = 0;
    this->heading = s;
    s = strchr(s,',');
    *(s++) = 0;
    this->dist = s;
    
    return 0;
  }

  return 1;
}


Datum pts[MAX_PTS];
int ptCnt;
char line[4096];

// read line and convert from UTF-16
int readLine(FILE *fp)
{
  int i=0;
  for (;;) {
    char c0,c1;
    if ((fread(&c0,1,1,fp) == 1) && 
	(fread(&c1,1,1,fp) == 1)) {
      if (c1 != 0) {
	return 2;
      }
      if (c0 == 0x0a) {
	line[i++] = '\0';
	return 0;
      }
      if (c0 == '\t') {
	line[i++] = ',';
      }
	  else if ((c0 != 0x0d) && (c0 != '"')) {
	line[i++] = c0;
      }

    }
    else {
      return 1;
    }
  }
}


int readCSV(char *fn)
{
  FILE *fp = fopen(fn,"rb");
  if (fp) {
    // throw away header FFFE
    if (fread(line,2,1,fp) != 1) {
      return 1;
    }
	readLine(fp); // throw away header line
    while (!readLine(fp)) {
      if (pts[ptCnt++].Set(line)) {
	printf("Out of Memory! point %d\n",ptCnt);
	return 2;
      }
    }
    fclose(fp);
	return 0;
  }
  return 1;
}

double ft2m(char *sft)
{
  double ft;
  sscanf(sft,"%lf",&ft);
  return ft * .30480;
}

void makeTrackPoint(Datum *dt)
{
  sprintf(line,"   <Trackpoint>\n    <Time>%sZ</Time>\n    <Position>\n     <LatitudeDegrees>%s</LatitudeDegrees>\n     <LongitudeDegrees>%s</LongitudeDegrees>\n    </Position>\n    <AltitudeMeters>%lf</AltitudeMeters>\n    <HeartRateBpm>\n     <Value>%s</Value>\n    </HeartRateBpm>\n   </Trackpoint>\n",dt->time,dt->lat,dt->lon,ft2m(dt->alt),dt->hr);
}
int writeTCX(char *fn)
{
  FILE *fp = fopen(fn,"wb");
  if (fp) {
    // write header
    fprintf(fp,"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n\
<TrainingCenterDatabase xmlns=\"http://www.garmin.com/xmlschemas/TrainingCenterDatabase/v2\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://www.garmin.com/xmlschemas/TrainingCenterDatabase/v2 http://www.garmin.com/xmlschemas/TrainingCenterDatabasev2.xsd\">\n<Activities>\n <Activity>\n");
    fprintf(fp,"  <Id>%sZ</Id>\n",pts[0].time);
    fprintf(fp,"  <Lap StartTime=\"%sZ\">\n",pts[0].time);
    fprintf(fp,"  <Track>\n");

    for (int i=0;i < ptCnt;i++) {
      makeTrackPoint(&pts[i]);
      // write track point
      if (fprintf(fp,line) != strlen(line)) {
	return 2;
      }
    }

    // write footer
    fprintf(fp,"  </Track>\n  </Lap>\n </Activity>\n</Activities>\n</TrainingCenterDatabase>\n");

    fclose(fp);
    return 0;
  }
  return 1;
}

int main(int argc,char *argv[])
{
  printf("Lincomatic GPS Master CSV to GPX Converter v0.1\n\n");
  if (argc != 3) {
    printf("Usage: csv2tcx infile outfile\n");
    return 1;
  }

  printf("Converting %s -> %s\n",argv[1],argv[2]);

  ptCnt = 0;
  if (!readCSV(argv[1])) {
    writeTCX(argv[2]);
  }

  printf("Trackpoints %d\n", ptCnt);

  return 0;
}
