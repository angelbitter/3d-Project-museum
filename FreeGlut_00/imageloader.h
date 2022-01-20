#ifndef IMAGE_LOADER_H_INCLUDED
#define IMAGE_LOADER_H_INCLUDED

// Intentamos imitar a una imagen
class Image {
public:
	Image(char* ps, int w, int h);
	~Image();

	// pixels es una array con la estructura (R1, G1, B1, R2, G2, B2, ....) donde cada terna RGB consiste en 
	// los datos de color de su respectivo pixel, estos colores oscilan de 0 a 255.
	// La array comienza por el pixel de abajo a la izquierda, continua hasta la derecha del todo, viaja una columna
	// hacia arriba y sigue repitiendo este proceso, este es el formato en que OpenGL admite imagenes sin errores
	char* pixels;
	int width;
	int height;
};

// Lee un bitmap de un fichero
Image* loadBMP(const char* filename);

#endif


