#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-page.h>
#include <poppler/cpp/poppler-image.h>
#include <poppler/cpp/poppler-page-renderer.h>
#include <tesseract/baseapi.h>
#include <tesseract/osdetect.h>
#include <leptonica/allheaders.h>
#include <iostream>
#include <memory>
#include <vector>
#include <string>

constexpr int ColorBytes = 4;
constexpr int DPI = 200;

// Converte poppler::image para Pix (32bpp RGBA → Leptonica)
Pix* convert_poppler_image_to_pix(const poppler::image& img, int num)
{
    if (!img.is_valid()) return nullptr;

    int width = img.width();
    int height = img.height();
    Pix* pix = pixCreate(width, height, 32); // RGBA

    std::cout << "Width:" << width << ", Height:" << height << ", Row per bytes:" << img.bytes_per_row() << std::endl;

    const char* colors = img.const_data();
    int c = 0;

    // l_uint32 *data = pixGetData(pix);
    // int wpl = pixGetWpl(pix);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const char r = colors[c++];
            const char g = colors[c++];
            const char b = colors[c++];
            // std::cout << "A: " << static_cast<int>(a) << "R: " << static_cast<int>(r) << "G: " << static_cast<int>(g) << "B: " << static_cast<int>(b) << std::endl;
            uint32_t rgba = (r << 24) | (b << 16) | (g << 8) | 255;
            pixSetPixel(pix, x, y, rgba);
            // data[y * wpl + x] = rgba;
        }
    }

    pixSetResolution(pix, DPI, DPI);

    std::string filename = "/home/bruno.lebtag/Projects/CplusplusTest/ocr/pages/page";
    filename += std::to_string(num);
    filename += ".png";

    if (pixWritePng(filename.c_str(), pix, 0.0)) {
        fprintf(stderr, "Erro ao salvar a imagem como PNG.\n");
        pixDestroy(&pix);
        return nullptr;
    }

    return pix;
}

int main(int argc, char* argv[])
{
    const std::string pdfPath = argv[1];
    std::unique_ptr<poppler::document> doc(poppler::document::load_from_file(pdfPath));
    if (!doc) {
        std::cerr << "Erro ao carregar o PDF.\n";
        return 1;
    }

    tesseract::TessBaseAPI ocr;

    // Inicializa com múltiplos idiomas (português e inglês, por exemplo)
    if (ocr.Init("./tessdata", "por")) {
        std::cerr << "Erro ao inicializar Tesseract com idiomas.\n";
        return 1;
    }

    // Habilita OSD (Orientation & Script Detection)
    ocr.SetPageSegMode(tesseract::PSM_AUTO_OSD);

    for (int i = 0; i < doc->pages(); ++i) {
        std::unique_ptr<poppler::page> page(doc->create_page(i));
        if (!page) continue;

        poppler::rectf page_rect = page->page_rect();
        double width_pt = page_rect.width();
        double height_pt = page_rect.height();

        int width_px = static_cast<int>(width_pt * DPI / 72.0);
        int height_px = static_cast<int>(height_pt * DPI / 72.0);

        printf("Page rotation: %d\n", page->orientation());

        poppler::page_renderer renderer;
        renderer.set_image_format(poppler::image::format_rgb24);
        renderer.set_render_hint(poppler::page_renderer::text_antialiasing);

        poppler::image img = renderer.render_page(page.get(), static_cast<double>(DPI), static_cast<double>(DPI), 0, 0, width_px, height_px, poppler::rotate_0);

        if (!img.is_valid()) {
            std::cerr << "Falha ao renderizar página " << i << "\n";
            continue;
        }

        Pix* pix = convert_poppler_image_to_pix(img, i + 1);
        if (!pix) {
            std::cerr << "Erro ao converter imagem da página " << i << "\n";
            continue;
        }

        ocr.SetImage(pix);

        // Detecta orientação e script (idioma)
        int orientation, direction, order, deskew;
        tesseract::OSResults osr;
        if (ocr.DetectOS(&osr)) {
            orientation = osr.best_result.orientation_id;
            std::cout << "Orientação detectada (página " << i+1 << "): " << orientation << " graus\n";
        }

        char* text = ocr.GetUTF8Text();
        std::cout << "Texto extraído da página " << (i + 1) << ":\n" << text << "\n";

        delete[] text;
        pixDestroy(&pix);
    }

    ocr.End();
    return 0;
}