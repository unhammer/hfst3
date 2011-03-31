#ifndef _LOOKUP_FLAG_DIACRITICS_H_
#define _LOOKUP_FLAG_DIACRITICS_H_
#include <set>
#include <map>
#include <string>
#include <cassert>
#include <vector>
#include <iostream>
#ifdef DEBUG
#include <iostream>
#endif

enum DiacriticOperator
  {
    Pop, Nop, Dop, Rop, Cop, Uop
  };

typedef std::map<std::string,DiacriticOperator> DiacriticOperators;
typedef std::map<std::string,std::string> DiacriticFeatures;
typedef std::map<std::string,std::string> DiacriticValues;
typedef std::map<std::string,bool> DiacriticSettingMap;
typedef std::map<std::string,std::string> FeatureValues;
typedef std::map<std::string,bool> FeaturePolarities;
typedef std::vector<std::string> StringVector;
class  FlagDiacriticTable
{
  static DiacriticOperators diacritic_operators;
  static DiacriticFeatures diacritic_features;
  static DiacriticValues diacritic_values;
  FeatureValues feature_values;
  FeaturePolarities feature_polarities;
  static DiacriticSettingMap diacritic_has_value;
  bool error_flag;
  static bool is_genuine_diacritic(const std::string &diacritic_string);
  static void split_diacritic(const std::string &diacritic_string);
  void set_positive_value(std::string &feature,
			  std::string &value);
  void set_negative_value(std::string &feature,
			  std::string &value);
  void disallow(std::string &feature,
		std::string &value);
  void disallow(std::string &feature);
  void require(std::string &feature,
	       std::string &value);
  void require(std::string &feature);
  void unify(std::string &feature,
	     std::string &value);
  void clear(std::string &feature);

 public:
  FlagDiacriticTable(void);
  /*static void define_diacritic(short diacritic_number,
			       const std::string &diacritic_string);
			       void insert_number(short key_number);*/
  void insert_symbol(const std::string &symbol);
  void reset(void);
  bool fails(void);
  static bool is_diacritic(const std::string &symbol);
  bool is_valid_string(const StringVector & input_string);
  StringVector filter_diacritics(const StringVector & input_string);
#ifdef DEBUG
  static void display(short diacritic); 
#endif
};

#endif
