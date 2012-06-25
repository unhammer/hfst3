#include "LexicalModelBuilder.h"

#ifndef MAIN_TEST

using hfst::HfstOutputStream;
using hfst::HFST_OLW_TYPE;
using hfst::TROPICAL_OPENFST_TYPE;

#define NUMBER_OF_LEXICAL_MODELS 4

LexicalModelBuilder::LexicalModelBuilder(std::istream &in)
{
  ModelBuilder::verbose_printf("Compiling lexical model.\n");
  for (size_t i = 0; i < NUMBER_OF_LEXICAL_MODELS; ++i)
    {
      ModelBuilder model_builder(WeightedStringVectorCollection(in,LEXICAL));
      model_vector.push_back(model_builder.get_model());
    }
}

void LexicalModelBuilder::store(const std::string &file_name)
{
  HfstOutputStream out(file_name,HFST_OLW_TYPE);

  HfstTransducer lexical_model(TROPICAL_OPENFST_TYPE);

  ModelBuilder::verbose_printf("Disjuncting lexical models and transforming "
			       "to optimized format.\n");

  for (TransducerVector::iterator it = model_vector.begin();
       it != model_vector.end();
       ++it)
    { lexical_model.disjunct(*it); }
  
  lexical_model.convert(HFST_OLW_TYPE); 
 
  ModelBuilder::verbose_printf(std::string("Storing lexical model in") +
			       file_name + ".\n");
 
  out << lexical_model;
}

void LexicalModelBuilder::store(void)
{
  HfstOutputStream out(HFST_OLW_TYPE);

  HfstTransducer lexical_model(TROPICAL_OPENFST_TYPE);

  ModelBuilder::verbose_printf("Disjuncting lexical models and transforming "
			       "to optimized format.\n");

  for (TransducerVector::iterator it = model_vector.begin();
       it != model_vector.end();
       ++it)
    { lexical_model.disjunct(*it); }
  
  lexical_model.convert(HFST_OLW_TYPE); 
 
  ModelBuilder::verbose_printf("Storing lexical model in <stdout>.\n");
 
  out << lexical_model;
}

#else // MAIN_TEST

int main(void)
{

}

#endif // MAIN_TEST
