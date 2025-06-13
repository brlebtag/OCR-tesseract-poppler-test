# OCR-tesseract-poppler-test
OCR Test using tesseract and poppler to extract text from PDF.

# Traineddata

Download from [here](https://github.com/tesseract-ocr/tessdata) and place them at folder "tessdata/" the following files:

* osd.traineddata
* [lang].traineddata /* It can be en.traineddata for english, por.traineddata for portuguese, and so on as desired... */

# Compile & Run
vcpkg install

cmake -B build

cmake --build build

./build/OcrTest file.pdf
