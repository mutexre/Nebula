#include <ApplicationServices/ApplicationServices.h>
#include <Nebula/Nebula.h>

const char* getColorSpaceModelString(CGColorSpaceModel model) {
    switch (model) {
        case kCGColorSpaceModelUnknown: return "Unknown";
        case kCGColorSpaceModelMonochrome: return "Monochrome";
        case kCGColorSpaceModelRGB: return "RGB";
        case kCGColorSpaceModelCMYK: return "CMYK";
        case kCGColorSpaceModelLab: return "LAB";
        case kCGColorSpaceModelDeviceN: return "DeviceN";
        case kCGColorSpaceModelIndexed: return "Indexed";
        case kCGColorSpaceModelPattern: return "Pattern";
    };
    return "None";
}

void printCGColorSpaceInfo(CGColorSpaceRef colorSpace) {
    
    printf("number of components: %lu\n", CGColorSpaceGetNumberOfComponents(colorSpace));
    printf("model: %s\n", getColorSpaceModelString(CGColorSpaceGetModel(colorSpace)));
}

const char* getCGImageAlphaInfoString(CGImageAlphaInfo alphaInfo) {
    switch (alphaInfo) {
        case kCGImageAlphaNone: return "None";
        case kCGImageAlphaPremultipliedLast: return "Prumultiplied Last (E.g.: premultiplied RGBA)";  /* For example, premultiplied RGBA */
        case kCGImageAlphaPremultipliedFirst: return "Premultiplied First (E.g.: premultiplied ARGB"; /* For example, premultiplied ARGB */
        case kCGImageAlphaLast: return "Last (E.g.: non-premultiplied RGBA)";                         /* For example, non-premultiplied RGBA */
        case kCGImageAlphaFirst: return "First (E.g.: non-premultiplied ARGB)";                       /* For example, non-premultiplied ARGB */
        case kCGImageAlphaNoneSkipLast: return "None, skip last (E.g.: RGBX)";                        /* For example, RBGX. */
        case kCGImageAlphaNoneSkipFirst: return "None, skip first (E.g.: XRGB)";                      /* For example, XRGB. */
        case kCGImageAlphaOnly: return "Alpha Only";
    };
    return "Unknown";
}

void printCGImageInfo(CGImageRef image) {
    printf("bits per component: %lu\n", CGImageGetBitsPerComponent(image));
    printf("bits per pixel: %lu\n", CGImageGetBitsPerPixel(image));
    printf("bytes per row: %lu\n", CGImageGetBytesPerRow(image));
    printf("alpha info: %s\n", getCGImageAlphaInfoString(CGImageGetAlphaInfo(image)));
    printCGColorSpaceInfo(CGImageGetColorSpace(image));
}

void getPixelData(CGImageRef imageRef, RT::u1* pixelData) {
    auto width = CGImageGetWidth(imageRef);
    auto height = CGImageGetHeight(imageRef);
    auto colorSpace = CGColorSpaceCreateDeviceRGB();
    RT::u4 bytesPerPixel = 4;
    auto bytesPerRow = bytesPerPixel * width;
    RT::u4 bitsPerComponent = 8;

    CGContextRef context = CGBitmapContextCreate(pixelData, width, height,
                                                 bitsPerComponent, bytesPerRow, colorSpace,
                                                 kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
    CGColorSpaceRelease(colorSpace);

    CGContextDrawImage(context, CGRectMake(0, 0, width, height), imageRef);
    CGContextRelease(context);
}

Nebula::Color::RGB<RT::u1> computeAvarageColor(const RT::u1* pixelData,
                                               size_t bytesPerRow, size_t bytesPerPixel,
                                               size_t rectX, size_t rectY,
                                               size_t rectWidth, size_t rectHeight)
{
    RT::u4 rSum = 0, gSum = 0, bSum = 0;

    for (auto y = 0; y < rectHeight; y++) {
        auto rowDataOffset = bytesPerRow * (rectY + y) + bytesPerPixel * rectX;
        for (auto x = 0; x < rectWidth; x += 1) {
            auto pixelDataOffset = rowDataOffset + bytesPerPixel * x;
            rSum += pixelData[pixelDataOffset + 2];
            gSum += pixelData[pixelDataOffset + 1];
            bSum += pixelData[pixelDataOffset + 0];
        }
    }

    auto numberOfPixels = rectWidth * rectHeight;
    return Nebula::Color::RGB<RT::u1>((rSum / numberOfPixels) & 0xff,
                                      (gSum / numberOfPixels) & 0xff,
                                      (bSum / numberOfPixels) & 0xff);
}

RT::u4 Ambilight::calculateBarSize(RT::u4 n, RT::u4 i, RT::u4 size, float a) {
    return zoneSize;
    auto half_n = 0.5f * float(n);
    auto f = (1.0f - a) * (1.0f - fabs(float(i) - half_n) / half_n) + a;
    return (size / 2) * f;
}

void Ambilight::computeColors(Nebula::Color::RGB<RT::u1>* leds, const RT::u1* pixelData,
                              size_t bytesPerRow, size_t bytesPerPixel,
                              RT::u4 width, RT::u4 height,
                              RT::u2 left, RT::u2 right,
                              RT::u2 bottom, RT::u2 top)
{
    RT::u4 ledIndex = 0;

    for (auto y = left - 1; y >= 0; y--)
        leds[ledIndex++] = computeAvarageColor(pixelData,
                                               bytesPerRow, bytesPerPixel,
                                               0, y * height / left,
                                               calculateBarSize(left, y, width, 0.3f), height / left);

    for (auto x = 0; x < top; x++)
        leds[ledIndex++] = computeAvarageColor(pixelData,
                                               bytesPerRow, bytesPerPixel,
                                               x * width / top, 0,
                                               width / top, calculateBarSize(top, x, height, 0.3f));

    for (auto y = 0; y < right; y++)
        leds[ledIndex++] = computeAvarageColor(pixelData,
                                               bytesPerRow, bytesPerPixel,
                                               width - 200, y * height / right,
                                               calculateBarSize(right, y, width, 0.3f), height / right);
}

Ambilight::Ambilight(RT::u4 numberOfLeds, RT::u4 zoneSize) : Generator(numberOfLeds) {
    this->zoneSize = zoneSize;
}

void Ambilight::generate(Nebula::Color::RGB<RT::u1>* output) {
    CGImageRef image = CGDisplayCreateImage(kCGDirectMainDisplay);
    if (!image) RT::error(0x01BB3D1E);

    CFDataRef data = CGDataProviderCopyData(CGImageGetDataProvider(image));
    const RT::u1* pixelData = CFDataGetBytePtr(data);

    auto width = (RT::u4)CGImageGetWidth(image);
    auto height = (RT::u4)CGImageGetHeight(image);

    auto colorTransformation = bind(Nebula::Color::transformSaturationOfRgb,
                                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
                                    [=](float saturation) -> float { return powf(saturation, 0.3f); });

    computeColors(output, pixelData,
                  CGImageGetBytesPerRow(image), CGImageGetBitsPerPixel(image) >> 3,
                  width, height,
                  9, 9, 0, 12,
                  colorTransformation);

    CFRelease(data);
    CGImageRelease(image);
}
