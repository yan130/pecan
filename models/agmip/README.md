A package to couple the AgMIP model execution utilities with the PEcAn model analysis utilities
==========================================================================

Steps to add AgMIP to PEcAn:

1. add modeltype to BETY
2. add a model and PFT to BETY for use with modeltype
3. implement 3 functions (described below and in documentation)
  * `write.configs.AgMIP()`
    * converts parameters & writes config / parameter files
    * identifies where met is
    * writes script to launch a single run
  * `met2model.AgMIP()`
  * `model2netcdf.AgMIP()`
    * converts output to PEcAn standard (netcdf format)
  * PEcAn --> AgMIP ICASA variable mappings are tracked in [this spreadsheet](https://docs.google.com/spreadsheets/d/1cXnf6Fku2NkfA3i-VZBXxvsdEvXoXeV4tC9eAZtiazY/edit#gid=0).
4. Add tests to `tests/testthat`
5. Update README, documentation
6. execute pecan with new model

More detailed instructions can be found in the documentation: https://pecanproject.github.io/pecan-documentation/master/adding-an-ecosystem-model.html

PEcAn - AgMIP coupling discussions and plans are in [this google doc](https://docs.google.com/document/d/1bY_4espfIilvvA9wmDkA6LfVDtGm59sUbz23u8bmlZE/edit#)

### Three Functions

There are 3 functions that will need to be implemented, each of these
functions will need to have MODEL be replaced with the actual modeltype as
it is defined in the BETY database.

* `write.config.MODEL.R`

 This will write the configuratin file as well as the job launcher used by
 PEcAn. There is an example of the job execution script in the template
 folder. The configuration file can also be a template that is found based
 on the revision number of the model. This should use the computed results
 specified in defaults and trait.values to write a configuration file
 based on the PFT and traits found.

* `met2model.MODEL.R`

 This will convert the standard Met CF file to the model specific file
 format. This will allow PEcAn to create metereological files for the
 specific site and model. This will only be called if no meterological
 data is found for that specific site and model combination.

* `model2netcdf.MODEL.R`

 This will convert the model specific output to NACP Intercomparison
 format. After this function is finished PEcAn will use the generated
 output and not use the model specific outputs. The outputs should be
 named YYYY.nc

### Additional Changes
 
* `README.md` 
 
This file should contain basic background information about the model. 
At a minimum, this should include the scientific motivation and scope, 
name(s) of maintainer(s), links to project homepage, and a list of a few
key publications. 

* `/tests/testthat/`

Each package should have tests that cover the key functions of the package, 
at a minimum, the three functions above.

* documentation

Update the `NAMESPACE`, `DESCRIPTION` and `man/*.Rd` files by running 

```r
devtools("models/<modelname>/")
```
