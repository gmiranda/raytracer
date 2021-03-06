#include <assert.h>
#include "raytracer.h"
#include "image.h"

#include "cmaterial.hxx"
#ifndef __WIN32__
#include <unistd.h>
#endif

#include "estats.hxx"

/*-<==>-----------------------------------------------------------------
  /
  /----------------------------------------------------------------------*/
CRayTracer::CRayTracer()
{
  max_recursion_level = 15;
}

/*-<==>-----------------------------------------------------------------
  / Create an image, and for each pixel in the screen create a line
  / and retrieve which color is seen through this line
  / Save the image to a file
  /----------------------------------------------------------------------*/
void CRayTracer::render() {
  // ...
    printf("\n                                          \r|=00");
  CBitmap resultat(camera.getXRes(), camera.getYRes(), 24);

  CLine raig;

  for(int i=0;i<camera.getXRes();i++)
    {
      if(i%15==0)
	{
	  printf("\b\b\b=>%d",100*i/camera.getXRes());
	  fflush(stdout);
	}
      for(int j=0;j<camera.getYRes();j++)
	{
	  raig=CLine(camera.getLineAt(i,j));
	  raig.color=VECTOR(0,0,0);

	  Estats::getInstance().incCamera();
	  trace(raig);

	  /*
	  // lo que MMX ho fa sol ho fem a m� :)
	  //saturaci�
	  raig.color.x=(raig.color.x>1)?1:raig.color.x;
	  raig.color.y=(raig.color.y>1)?1:raig.color.y;
	  raig.color.z=(raig.color.z>1)?1:raig.color.z;
	  //
	  raig.color.x=(raig.color.x<0)?0:raig.color.x;
	  raig.color.y=(raig.color.y<0)?0:raig.color.y;
	  raig.color.z=(raig.color.z<0)?0:raig.color.z;
	  */

	  resultat.setPixel(i, j,raig.color);
	}
    }

  resultat.saveTGA("res.tga");
  printf("\n\n");
}

/*-<==>-----------------------------------------------------------------
  / Find which object and at which 't' the line hits and object
  / from the scene.
  / Returns true if the object hits some object
  /----------------------------------------------------------------------*/
bool CRayTracer::intersects(CLine &line)
{
  Estats::getInstance().incIntersects();

  // Example of traversing all the objects registered in the scene
  // Same thing for lights
  LRTObjects::iterator i = objects.begin();

  //hem de buscar la mes propera
  //em sembla que aquest es el problema
  //aqui pillem el primer objecte, sigui davant o darrere,
  //tapat per un altre o no...
  SCALAR t=-1;

  bool init=false;

  line.t=-1;

  //per si les mosquis
  if(objects.begin()==objects.end())
  {
      line.t=-1;
      return false;
  }

  while( i != objects.end() )
    {
      CRTObject *obj = *i++;
      // Si la linea intersecta al objeto
      if(obj->hits(line,t))
	{
	  //si es el primer element que mirem
	  if(!init)
	    {
	      line.obj=obj;
	      line.t=t;
	      init=true;
	    }
	  else if(line.t>t)
	    {
	      //std::cerr << "Inteseccion, amigo conductor" << std::endl;
	      // Guardamos el objeto con el que chocamos
	      line.obj=obj;
	      // Guardamos la distancia de intersecci�n
	      line.t=t;
	    }
	}
    }


  if(line.t>0)
    {
      Estats::getInstance().incIntersectsOK();
      return true;
    }
  else
    {
      Estats::getInstance().incIntersectsNO();
      return false;
    }
}

/*-<==>-----------------------------------------------------------------
  / Returns in line.color the color captured by the line.
  /----------------------------------------------------------------------*/
void CRayTracer::trace(CLine &line)
{
  SCALAR t;

  //si no hem arribat al maxim de recursi�
  if (line.getLevel()>max_recursion_level)
    return;

  Estats::getInstance().incLine();

  //un nivell mes
  ++line;

  //fiquem com si no per inicialitzar
  t=line.t=-1;

  //si no intersecta no ens interesa
  if(!intersects(line))
    {
      this->background(line);
      return;
    }

  VECTOR pos = line.getIntersection();

  //Ambiental suposem que hi es encara que no llum
  line.addColor(line.obj->getMaterial()->getDiffuseColor(pos)
		*1*
		(1-line.obj->getMaterial()->getReflectance(pos)));

  //llums
  std::list<CLight *>::iterator llum;

  for(llum = lights.begin();llum!=lights.end();++llum)
    {
      CLight &ellum=(**llum);
      //fem per sombres
      //CLine llumLinea(line.loc, ellum.getLocation()-line.loc);
      CLine llumLinea(ellum.getLocation(), (pos-ellum.getLocation()));

      llumLinea.t=-1;

      // Miramos si esta luz intersecta con el punto
      //intersects(llumLinea);
      // Si asi es, no hay sombra
      //line.obj->hits(llumLinea,t);
		/*bool sombra = false;
		// Miramos los objetos
		LRTObjects::iterator i;
		for(i = objects.begin();i!=objects.end();++i){
			CRTObject* obj = *i;
			// Si hay un objeto entre la fuente de luz y el punto
			if(obj->hits(llumLinea,t)){
				// Hay sombra
				sombra=true;
				break;
			}
		}*/
      if(line.obj->hits(llumLinea,t)){
	if(t>0.0)
	  {
	    //posem les coses simples
	    //si el vector a la llum desde on xoquem
	    //no intersecta amb algo no ilumina
	    //si ho fa pero darrere, no toca
	    //sino si
	    //i es el que son les condicions
	    //
	    //pq no fa sombres...
	    //un misteri
	    //
		    //printf("%d\n",t);
		  // Vector L
		  VECTOR L = ellum.getLocation()-pos;
		  L.normalize();

		  VECTOR N = line.obj->getNormal(pos);
		  SCALAR NL=N.dot(L);

		  if(NL<0) NL=0;


		  //llum difosa
		  VECTOR color=line.obj->getMaterial()->getDiffuseColor(pos)
			*
			(NL)
			*
			(1-line.obj->getMaterial()->getReflectance(pos));

		  color.x*=ellum.getColor().x;
		  color.y*=ellum.getColor().y;
		  color.z*=ellum.getColor().z;

		  line.addColor(color);

		  //llum especular
		  VECTOR E;
		  E=-line.dir;
		  //E.normalize();
		  // La L paralela
		  VECTOR Lpar =N*(N.dot(L));
		  // La L perpendicular
		  VECTOR Lper = (L-Lpar);
		  VECTOR R = Lpar-Lper;
		  // En principio no hace falta
		  //R.normalize();
		  // cos Beta = RE, se calcula asi :)
		  SCALAR RE=R.dot(E);
		  // Si es negativo, no hay componente especular
		  if(RE>=0){
			// Especular = Is*(cos Beta)^n por Ks
			// Dice que podemos sudar de Is y Ks xD
			// Ademas, 20 o 21 es un 'numbero sunficiete'
			COLOR especular=VECTOR(1.0,1.0,1.0)
			  *pow(RE,81)/**0.8f*/;

			//depen de la reflectance tindra un brillo mes o menys
			especular.x*=(1-line.obj->getMaterial()->getReflectance(pos));
			especular.y*=(1-line.obj->getMaterial()->getReflectance(pos));
			especular.z*=(1-line.obj->getMaterial()->getReflectance(pos));

			especular.x*=ellum.getColor().x;
			especular.y*=ellum.getColor().y;
			especular.z*=ellum.getColor().z;

			line.addColor(especular);
		  }

		  if(t>100)
		    {
		      //atenuo per dist�ncia
		      if(t>=1000)
			line.color=VECTOR(0,0,0);
		      else
			{
			  SCALAR at=1-(t/1000);
			  line.color.x*=at;
			  line.color.y*=at;
			  line.color.z*=at;
			}
		    }


		}
    }
      else
      {
        //sombra
         Estats::getInstance().incSombra();
	 //background(line.color);
      }
    }
	// Factor de refraccion (se usara luego)
	SCALAR factor = line.obj->getMaterial()->getRefraction(pos);
	//reflexe / sombra
	if(line.obj->getMaterial()->getReflectance(pos)>0.0f)
	{

		CLine reflexe;

		//line.t=-1;

		reflexe= line.getReflected(pos,line.obj->getNormal(pos) );
		reflexe.loc-=line.dir*SMALL_AMOUNT;

		Estats::getInstance().incReflexe();
		trace(reflexe);

		//aixi rulez
		line.addColor(reflexe.color*0.5*(line.obj->getMaterial()->getReflectance(pos)));
	}
	// Refraccion
	else if(factor>0.0f)
	{
		// Factor es el indice de refraccion del medio.
		// El del cristal es algo asi como 1.52, asi que se le pasa 1.0/1.52


		CLine refractada=

		line.getRefracted(
			pos,
			line.obj->getNormal(pos),
			1.0/line.obj->getMaterial()->getRefraction(pos)
			);

		Estats::getInstance().incRefrac();

		// Trazamos la refraccion
		trace(refractada);
		// Estadisticas

		line.addColor(refractada.color*0.5);
	}
}

/*-<==>-----------------------------------------------------------------
  / Default background
  /----------------------------------------------------------------------*/
void CRayTracer::background(CLine &line)
{
  line.color = COLOR (0.2,0.2,0.8);
  //line.color = COLOR(0.0,0.0,0.0);
}

