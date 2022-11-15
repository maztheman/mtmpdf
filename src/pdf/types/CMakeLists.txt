set(SRCS
	ClippingPath.cpp   
	GraphicsState.cpp  
	Line.cpp           
	Matrix.cpp         
	MultiColor.cpp     
	PathState.cpp      
	PdfColumn.cpp      
	PdfPoint.cpp       
	PdfRect.cpp        
	TextAtLocation.cpp 
	TextState.cpp      
	ClippingPath.h     
	GraphicsState.h    
	Line.h             
	Matrix.h           
	MultiColor.h       
	PathState.h        
	PdfColumn.h        
	PdfPoint.h         
	PdfRect.h          
	TextAtLocation.h   
	TextState.h        
)

target_sources(pdflib PRIVATE ${SRCS})