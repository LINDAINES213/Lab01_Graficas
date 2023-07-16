#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>

class Color {
public:
    unsigned char r, g, b;
    // Constructor con valores predeterminados
    Color(unsigned char red = 0, unsigned char green = 0, unsigned char blue = 0)
            : r(red), g(green), b(blue) {}
};

struct Vertex2 {
    int x, y;

    // Constructor
    Vertex2(int xPos, int yPos)
            : x(xPos), y(yPos) {}
};

// Función para limpiar el framebuffer
void clear(unsigned char* framebuffer, const Color clearColor, int width, int height) {
    for (int i = 0; i < width * height * 3; i += 3) {
        framebuffer[i] = clearColor.b;
        framebuffer[i + 1] = clearColor.g;
        framebuffer[i + 2] = clearColor.r;
    }
}

// Función para colocar un punto en el framebuffer
void point(unsigned char* framebuffer, const Vertex2 vertex, const Color currentColor, int width) {
    int index = (vertex.y * width + vertex.x) * 3;
    framebuffer[index] = currentColor.b;
    framebuffer[index + 1] = currentColor.g;
    framebuffer[index + 2] = currentColor.r;
}

void drawLine(unsigned char* framebuffer, const Vertex2& startPoint, const Vertex2& endPoint, const Color& lineColor, int width) {
    int startX = startPoint.x;
    int startY = startPoint.y;
    int endX = endPoint.x;
    int endY = endPoint.y;

    int deltaX = std::abs(endX - startX);
    int deltaY = std::abs(endY - startY);
    int signX = startX < endX ? 1 : -1;
    int signY = startY < endY ? 1 : -1;

    int error = deltaX - deltaY;
    int currentX = startX;
    int currentY = startY;

    while (currentX != endX || currentY != endY) {
        point(framebuffer, Vertex2(currentX, currentY), lineColor, width);

        int error2 = error * 2;
        if (error2 > -deltaY) {
            error -= deltaY;
            currentX += signX;
        }
        if (error2 < deltaX) {
            error += deltaX;
            currentY += signY;
        }
    }
}

void drawPolygon(unsigned char* framebuffer, const std::vector<Vertex2>& points, const Color& lineColor, int width, int borderWidth) {
    int numPoints = points.size();

    for (int i = 0; i < numPoints; i++) {
        const Vertex2& currentPoint = points[i];
        const Vertex2& nextPoint = points[(i + 1) % numPoints];

        for (int j = 0; j < borderWidth; j++) {
            drawLine(framebuffer, Vertex2(currentPoint.x - j, currentPoint.y), Vertex2(nextPoint.x - j, nextPoint.y), lineColor, width);
        }
    }
}


void fillPolygon(unsigned char* framebuffer, const std::vector<Vertex2>& vertices, const Color& fillColor, int width) {
    int numVertices = vertices.size();

    // Obtener las coordenadas mínimas y máximas para determinar el rango de escaneo
    int minY = vertices[0].y;
    int maxY = vertices[0].y;
    for (int i = 1; i < numVertices; i++) {
        int y = vertices[i].y;
        if (y < minY) {
            minY = y;
        }
        if (y > maxY) {
            maxY = y;
        }
    }

    // Escanear cada línea vertical y rellenar los píxeles dentro del polígono
    for (int y = minY; y <= maxY; y++) {
        std::vector<int> intersectionPoints;

        // Encontrar las intersecciones de la línea vertical con los bordes del polígono
        for (int i = 0; i < numVertices; i++) {
            const Vertex2& currentVertex = vertices[i];
            const Vertex2& nextVertex = vertices[(i + 1) % numVertices];

            int currentY = currentVertex.y;
            int nextY = nextVertex.y;

            // Comprobar si la línea vertical intersecta con el borde actual
            if ((currentY <= y && y < nextY) || (nextY <= y && y < currentY)) {
                // Calcular la coordenada x de la intersección
                int intersectionX = currentVertex.x + (nextVertex.x - currentVertex.x) * (y - currentY) / (nextY - currentY);
                intersectionPoints.push_back(intersectionX);
            }
        }

        // Ordenar las coordenadas x de las intersecciones en orden ascendente
        std::sort(intersectionPoints.begin(), intersectionPoints.end());

        // Rellenar los píxeles entre las intersecciones
        for (int i = 0; i < intersectionPoints.size(); i += 2) {
            int startX = intersectionPoints[i];
            int endX = intersectionPoints[i + 1];
            for (int x = startX; x < endX; x++) {
                point(framebuffer, Vertex2(x, y), fillColor, width);
            }
        }
    }
}



// Función para escribir un archivo BMP
void renderBuffer(const std::string& filename, int width, int height, unsigned char* framebuffer) {
    std::ofstream file(filename, std::ios::binary);

    // Encabezado del archivo BMP
    unsigned char fileHeader[] = {
            'B', 'M',  // Signature
            0, 0, 0, 0, // File size (will be filled later)
            0, 0,      // Reserved
            0, 0,      // Reserved
            54, 0, 0, 0 // Pixel data offset
    };

    // Encabezado de información del BMP
    unsigned char infoHeader[] = {
            40, 0, 0, 0,  // Info header size
            0, 0, 0, 0,   // Image width (will be filled later)
            0, 0, 0, 0,   // Image height (will be filled later)
            1, 0,         // Number of color planes
            24, 0,        // Bits per pixel (3 bytes)
            0, 0, 0, 0,   // Compression method
            0, 0, 0, 0,   // Image size (will be filled later)
            0, 0, 0, 0,   // Horizontal resolution (pixels per meter, not used)
            0, 0, 0, 0,   // Vertical resolution (pixels per meter, not used)
            0, 0, 0, 0,   // Number of colors in the palette (not used)
            0, 0, 0, 0    // Number of important colors (not used)
    };

    // Calculate some values
    int imageSize = width * height * 3;  // 3 bytes per pixel (BGR)
    int fileSize = imageSize + sizeof(fileHeader) + sizeof(infoHeader);

    // Fill in the file header
    *(int*)&fileHeader[2] = fileSize;          // File size
    *(int*)&fileHeader[10] = sizeof(fileHeader) + sizeof(infoHeader);  // Pixel data offset

    // Fill in the info header
    *(int*)&infoHeader[4] = width;
    *(int*)&infoHeader[8] = height;
    *(int*)&infoHeader[20] = imageSize;

    // Write the headers to the file
    file.write(reinterpret_cast<char*>(fileHeader), sizeof(fileHeader));
    file.write(reinterpret_cast<char*>(infoHeader), sizeof(infoHeader));

    // Write the pixel data
    file.write(reinterpret_cast<char*>(framebuffer), imageSize);

    // Close the file
    file.close();
}

// Función para renderizar la escena y guardar en un archivo BMP
void render(const std::string& filename, int width, int height) {
    unsigned char* framebuffer = new unsigned char[width * height * 3];

    Color clearColor(0, 0, 0);
    clear(framebuffer, clearColor, width, height);

    Color currentColor(255, 255, 255);

    // Dibujar la orilla del polígono (color blanco)
    Color borderColor(255, 255, 255);
    int borderWidth = 3;

    std::vector<Vertex2> polygon3Vertices = {
            Vertex2(377, 249),
            Vertex2(411, 197),
            Vertex2(436, 249)
    };

    // Relleno del polígono (color rojo)
    Color fillColor3(255, 0, 0);
    fillPolygon(framebuffer, polygon3Vertices, fillColor3, width);
    drawPolygon(framebuffer, polygon3Vertices, borderColor, width, borderWidth);

    std::vector<Vertex2> polygon2Vertices = {
            Vertex2(321, 335),
            Vertex2(288, 286),
            Vertex2(339, 251),
            Vertex2(374, 302)
    };

    // Relleno del polígono (color azul)
    Color fillColor(0, 0, 255);
    fillPolygon(framebuffer, polygon2Vertices, fillColor, width);
    drawPolygon(framebuffer, polygon2Vertices, borderColor, width, borderWidth);

    std::vector<Vertex2> polygon1Vertices = {
            Vertex2(165, 380),
            Vertex2(185, 360),
            Vertex2(180, 330),
            Vertex2(207, 345),
            Vertex2(233, 330),
            Vertex2(230, 360),
            Vertex2(250, 380),
            Vertex2(220, 385),
            Vertex2(205, 410),
            Vertex2(193, 383)
    };

    // Relleno del polígono (color amarillo)
    Color fillColor2(255, 255, 0);
    fillPolygon(framebuffer, polygon1Vertices, fillColor2, width);
    drawPolygon(framebuffer, polygon1Vertices, borderColor, width, borderWidth);

    renderBuffer(filename, width, height, framebuffer);

    delete[] framebuffer;
}

int main() {
    // Tamaño del framebuffer (ejemplo: 800x600)
    int width = 800;
    int height = 600;

    render("out.bmp", width, height);

    return 0;
}