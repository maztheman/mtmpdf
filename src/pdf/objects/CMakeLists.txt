set(SRCS
	PdfCatalog.h         
	PdfCIDToGIDMap.h     
	PdfDictionary.h      
	PdfDocument.h        
	PdfFont.h            
	PdfFontDescriptor.h  
	PdfObject.h          
	PdfObjectStream.h    
	PdfPage.h            
	PdfPages.h           
	PdfString.h          
	PdfXRef.h            
	PdfCatalog.cpp       
	PdfCIDToGIDMap.cpp   
	PdfDictionary.cpp    
	PdfDocument.cpp      
	PdfFont.cpp          
	PdfFontDescriptor.cpp
	PdfObject.cpp        
	PdfObjectStream.cpp  
	PdfPage.cpp          
	PdfPages.cpp         
	PdfString.cpp        
	PdfXRef.cpp          
)

target_sources(pdflib PRIVATE ${SRCS})