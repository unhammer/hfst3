//       This program is free software: you can redistribute it and/or modify
//       it under the terms of the GNU General Public License as published by
//       the Free Software Foundation, version 3 of the License.
//
//       This program is distributed in the hope that it will be useful,
//       but WITHOUT ANY WARRANTY; without even the implied warranty of
//       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//       GNU General Public License for more details.
//
//       You should have received a copy of the GNU General Public License
//       along with this program.  If not, see <http://www.gnu.org/licenses/>.

/* \file HfstApply.cc 
   \brief HFST transducer functions that take several parameters are 
   handled here. 
 */

#include "HfstTransducer.h"

#ifndef MAIN_TEST

namespace hfst
{
  // Whether weights or other information are lost in the conversion.
  bool HfstTransducer::is_safe_conversion
  (ImplementationType original, ImplementationType converted) {
    if (original == converted)
      return true;
    if (original == TROPICAL_OPENFST_TYPE && converted == LOG_OPENFST_TYPE) {
      return false;
    }
    if (original == LOG_OPENFST_TYPE && converted == TROPICAL_OPENFST_TYPE) {
      return false;
    }
    if (original == TROPICAL_OPENFST_TYPE || original == LOG_OPENFST_TYPE) {
      if (converted == SFST_TYPE) {
        return false;
      }
      if (converted == FOMA_TYPE) {
        return false;
      }
    }
    /* Add here your library. */
    //#ifdef HAVE_MY_TRANSDUCER_LIBRARY
    //if (original == MY_TRANSDUCER_LIBRARY_TYPE) {
    //  // From my transducer to weighted transducer
    //  if ( converted == TROPICAL_OPENFST_TYPE || 
    //       converted == LOG_OPENFST_TYPE) {
    //  return true;  // if your library supports weights
    //  return false; // if your library does not support weights
    //  }
    //  // From my transducer to unweighted
    //  else {
    //  return true;  // if your library does not support weights
    //  return false; // if your library supports weights
    // }
    //}
    //#endif
    return true;
  }

  HfstTransducer &HfstTransducer::apply(
#if HAVE_SFST
 SFST::Transducer * (*sfst_funct)(SFST::Transducer *), 
#endif
#if HAVE_OPENFST
 fst::StdVectorFst * (*tropical_ofst_funct)(fst::StdVectorFst *),
 hfst::implementations::LogFst * 
 (*log_ofst_funct)(hfst::implementations::LogFst *),
#endif
#if HAVE_FOMA
 fsm * (*foma_funct)(fsm *),
#endif
 /* Add your library. */
 //#if HAVE_MY_TRANSDUCER_LIBRARY
 //my_namespace::MyFst * (*my_transducer_library_funct)(my_namespace::MyFst *),
 //#endif
  bool foo )
    {
      (void)foo;
    switch(this->type)
      {
#if HAVE_SFST
      case SFST_TYPE:
        {
          SFST::Transducer * sfst_temp = 
            sfst_funct(implementation.sfst);
          delete implementation.sfst;
          implementation.sfst = sfst_temp;
          break;
        }
#endif
#if HAVE_OPENFST
      case TROPICAL_OPENFST_TYPE:
        {
          fst::StdVectorFst * tropical_ofst_temp =
            tropical_ofst_funct(implementation.tropical_ofst);
          delete implementation.tropical_ofst;
          implementation.tropical_ofst = tropical_ofst_temp;
          break;
        }
      case LOG_OPENFST_TYPE:
        {
          hfst::implementations::LogFst * log_ofst_temp =
            log_ofst_funct(implementation.log_ofst);
          delete implementation.log_ofst;
          implementation.log_ofst = log_ofst_temp;
          break;
        }
#endif
#if HAVE_FOMA
      case FOMA_TYPE:
        {
      fsm * foma_temp =
            foma_funct(implementation.foma);
          this->foma_interface.delete_foma(implementation.foma);
          implementation.foma = foma_temp;
          break;
        }
#endif
        /* Add your library here. */
        //#if HAVE_MY_TRANSDUCER_LIBRARY
        //case MY_TRANSDUCER_LIBRARY_TYPE:
        //{
        //  my_namespace::MyFst * my_fst_temp =
        //  my_transducer_library_funct(implementation.my_transducer_library);
        //delete implementation.my_transducer_library;
        //implementation.my_transducer_library = my_fst_temp;
        //break;
        //}
        //#endif
        case ERROR_TYPE:
        default:
          HFST_THROW(TransducerHasWrongTypeException);
      }
    return *this;
  }

  HfstTransducer &HfstTransducer::apply
  (
#if HAVE_SFST
SFST::Transducer * (*sfst_funct)(SFST::Transducer *, unsigned int n),
#endif
#if HAVE_OPENFST
   fst::StdVectorFst * (*tropical_ofst_funct)
(fst::StdVectorFst *, unsigned int n),
   hfst::implementations::LogFst * (*log_ofst_funct)
   (hfst::implementations::LogFst *, unsigned int n),
#endif
#if HAVE_FOMA
   fsm * (*foma_funct)(fsm *, unsigned int n),
#endif
/* Add your library here. */
//#if HAVE_MY_TRANSDUCER_LIBRARY
//   my_namespace::MyFst * 
//     (*my_transducer_library_funct)(my_namespace::MyFst *,int n),
//#endif
   unsigned int n )
  {
    switch(this->type)
      {
#if HAVE_SFST
      case SFST_TYPE:
        {
          SFST::Transducer * sfst_temp = 
            sfst_funct(implementation.sfst,n);
          delete implementation.sfst;
          implementation.sfst = sfst_temp;
          break;
        }
#endif
#if HAVE_OPENFST
      case TROPICAL_OPENFST_TYPE:
        {
          fst::StdVectorFst * tropical_ofst_temp =
            tropical_ofst_funct(implementation.tropical_ofst,n);
          delete implementation.tropical_ofst;
          implementation.tropical_ofst = tropical_ofst_temp;
          break;
        }
      case LOG_OPENFST_TYPE:
        {
          hfst::implementations::LogFst * log_ofst_temp =
            log_ofst_funct(implementation.log_ofst,n);
          delete implementation.log_ofst;
          implementation.log_ofst = log_ofst_temp;
          break;
        }
#endif
#if HAVE_FOMA
      case FOMA_TYPE:
        {
      fsm * foma_temp = 
            foma_funct(implementation.foma,n);
          this->foma_interface.delete_foma(implementation.foma);
          implementation.foma = foma_temp;
          break;
    }
#endif
        /* Add your library here. */
        //#if HAVE_MY_TRANSDUCER_LIBRARY
        //case MY_TRANSDUCER_LIBRARY_TYPE:
        //{
        //my_namespace::MyFst * my_fst_temp = 
        //  my_transducer_library_funct(implementation.my_transducer_library,n);
        //delete (implementation.my_transducer_library);
        //implementation.my_transducer_library = my_fst_temp;
        //break;
        //}
        //#endif
        case ERROR_TYPE:
        default:
          HFST_THROW(TransducerHasWrongTypeException);
      }
    return *this;
  }

  HfstTransducer &HfstTransducer::apply
  (
#if HAVE_SFST
   SFST::Transducer * (*sfst_funct)(SFST::Transducer *, String, String),
#endif
#if HAVE_OPENFST
   fst::StdVectorFst * (*tropical_ofst_funct)(fst::StdVectorFst *,String, 
                                              String),
   hfst::implementations::LogFst * (*log_ofst_funct)
   (hfst::implementations::LogFst *,String, String),
#endif
#if HAVE_FOMA
   fsm * (*foma_funct)(fsm *, String, String),
#endif
   /* Add your library here. */
   //#if HAVE_MY_TRANSDUCER_LIBRARY
   //my_namespace::MyFst * 
   // (*my_transducer_library_funct)(my_namespace::MyFst *, String, String),
   //#endif
   String s1, String s2)
  {
    switch(this->type)
      {
#if HAVE_SFST
      case SFST_TYPE:
        {
          SFST::Transducer * sfst_temp = 
            sfst_funct(implementation.sfst,s1,s2);
          delete implementation.sfst;
          implementation.sfst = sfst_temp;
          break;
        }
#endif
#if HAVE_OPENFST
      case TROPICAL_OPENFST_TYPE:
        {
          fst::StdVectorFst * tropical_ofst_temp =
            tropical_ofst_funct(implementation.tropical_ofst,s1,s2);
          delete implementation.tropical_ofst;
          implementation.tropical_ofst = tropical_ofst_temp;
          break;
        }
      case LOG_OPENFST_TYPE:
        {
          hfst::implementations::LogFst * log_ofst_temp =
            log_ofst_funct(implementation.log_ofst,s1,s2);
          delete implementation.log_ofst;
          implementation.log_ofst = log_ofst_temp;
          break;
        }
#endif
#if HAVE_FOMA
      case FOMA_TYPE:
        {
      fsm * foma_temp = 
            foma_funct(implementation.foma,s1,s2);
          this->foma_interface.delete_foma(implementation.foma);
          implementation.foma = foma_temp;
      break;
        }
#endif
        /* Add your library here. */
        //#if HAVE_MY_TRANSDUCER_LIBRARY
        //case MY_TRANSDUCER_LIBRARY_TYPE:
        //{
        //my_namespace::MyFst * my_fst_temp = 
        //  my_transducer_library_funct
        //    (implementation.my_transducer_library,s1,s2);
        //delete(implementation.my_transducer_library);
        //implementation.my_transducer_library = my_fst_temp;
        //break;
        //}
        //#endif
        case ERROR_TYPE:
        default:
          HFST_THROW(TransducerHasWrongTypeException);
      }
    return *this;
  }


  HfstTransducer &HfstTransducer::apply(
#if HAVE_SFST
   SFST::Transducer * (*sfst_funct)(SFST::Transducer *,
                                    SFST::Transducer *),
#endif
#if HAVE_OPENFST
   fst::StdVectorFst * (*tropical_ofst_funct)(fst::StdVectorFst *,
                                              fst::StdVectorFst *),
   hfst::implementations::LogFst * (*log_ofst_funct)
   (hfst::implementations::LogFst *,hfst::implementations::LogFst *),
#endif
#if HAVE_FOMA
   fsm * (*foma_funct)(fsm *,
                                    fsm *),
#endif
   /* Add your library here. */
   //#if HAVE_MY_TRANSDUCER_LIBRARY
   //my_namespace::MyFst * (*my_transducer_library_funct)(my_namespace::MyFst *,
   //                                    my_namespace::MyFst *),
   //#endif
   HfstTransducer &another, bool harmonize)
  {
    if (this->type != another.type)
      HFST_THROW(TransducerTypeMismatchException);

    HfstTransducer * another_;

    if (harmonize)
      {
        another_ =
          this->harmonize_(const_cast<HfstTransducer&>(another));
        
        if (another_ == NULL) // foma
          { 
            another_ = new HfstTransducer(another);
          }
      }
    else
      {
        another_ = new HfstTransducer(another);
      }

    switch (this->type)
      {
#if HAVE_SFST
      case SFST_TYPE:
        {
          SFST::Transducer * sfst_temp = 
            sfst_funct(implementation.sfst,
               another_->implementation.sfst);
          delete implementation.sfst;
          implementation.sfst = sfst_temp;
          break;
        }
#endif
#if HAVE_OPENFST
      case TROPICAL_OPENFST_TYPE:
        {
          /* if no harmonization was carried out, symbol encodnings must be harmonized                                                                                                                                                                       as OpenFst does not do it */
          if (! harmonize)
            {
              HfstTransducer * tmp = this->harmonize_symbol_encodings(*another_);
              delete another_;
              another_ = tmp;
            }

          fst::StdVectorFst * tropical_ofst_temp =
            tropical_ofst_funct(this->implementation.tropical_ofst,
                                another_->implementation.tropical_ofst);
          delete implementation.tropical_ofst;
          implementation.tropical_ofst = tropical_ofst_temp;
          break;
        }
      case LOG_OPENFST_TYPE:
        {
          hfst::implementations::LogFst * log_ofst_temp =
            log_ofst_funct(implementation.log_ofst,
                           another_->implementation.log_ofst);
          delete implementation.log_ofst;
          implementation.log_ofst = log_ofst_temp;
          break;
        }
#endif
#if HAVE_FOMA
      case FOMA_TYPE:
        {
          /* prevent foma from harmonizing, if needed */
          if (! harmonize)
            {
              this->insert_missing_symbols_to_alphabet_from(*another_);
              another_->insert_missing_symbols_to_alphabet_from(*this);
            }
          fsm * foma_temp = 
            foma_funct(implementation.foma,another_->implementation.foma);
          delete implementation.foma;
          implementation.foma = foma_temp;
          break;
        }
#endif
        /* Add your library here. */
        //#if HAVE_MY_TRANSDUCER_LIBRARY
        //case MY_TRANSDUCER_LIBRARY_TYPE:
        //{
        //  my_namespace::MyFst * my_fst_temp = 
        //    my_transducer_library_funct
        //      (implementation.my_transducer_library,
        //       another.implementation.my_transducer_library);
        //  delete implementation.my_transducer_library;
        //  implementation.my_transducer_library = my_fst_temp;
        //  break;
        //}
        //#endif
        case ERROR_TYPE:
        default:
          HFST_THROW(TransducerHasWrongTypeException);
      }
    delete another_;

    return *this;
  }
}

#else // MAIN_TEST was defined

#include <iostream>

int main(int argc, char * argv[])
{
    std::cout << "Unit tests for " __FILE__ ":" << std::endl;
    
    std::cout << "ok" << std::endl;
    return 0;
}

#endif // MAIN_TEST
