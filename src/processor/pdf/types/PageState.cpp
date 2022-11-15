#include "PageState.h"
#include <pdf/objects/PdfPage.h>
#include <pdf/objects/PdfFont.h>

namespace processor::pdf {

    SPageState::SPageState(CPdfPage* pPage, size_t pageNum)
        : page(pPage)
        , nPage(pageNum)
    {
        GS.CTM = Matrix{ 1.0, 0.0, 0.0, 1.0, 0.0, 792.0 };
        Multiply(GS.CTM, Matrix{ 1.0, 0.0, 0.0, -1.0, 0.0, 0.0 });

        if (page) {
            for (auto& f : page->m_Fonts) {
                FontMaps[f->m_sAlias] = f;
            }
        }
    }

}