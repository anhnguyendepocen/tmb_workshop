#include <TMB.hpp>

//// ---------- Custom likelihood functions, used be used in template
//// below. These are not built into TMB like dnorm and dgamma are.
//log-normal likelihood
template<class Type>
Type dlognorm(Type x, Type meanlog, Type sdlog, int give_log=0){
  //return 1/(sqrt(2*M_PI)*sd)*exp(-.5*pow((x-mean)/sd,2));
  Type logres = dnorm( log(x), meanlog, sdlog, true) - log(x);
  if(give_log) return logres; else return exp(logres);
}
// Inverse Gaussian
template<class Type>
Type dinvgauss(Type x, Type mean, Type shape, int give_log=0){
  Type logres = 0.5*log(shape) - 0.5*log(2*M_PI*pow(x,3)) - (shape * pow(x-mean,2) / (2*pow(mean,2)*x));
  if(give_log) return logres; else return exp(logres);
}

//// ---------- The CPUE model
template<class Type>
Type objective_function<Type>::operator() ()
{

  DATA_INTEGER(likelihood); 	// Likelihood flag
  DATA_VECTOR(y);		// Observed catches (can be zero)
  DATA_VECTOR(lat);		// latitude
  DATA_VECTOR(lon);		// longitude
  DATA_MATRIX(dd);		// pairwise distances

  // Parameters
  PARAMETER(intercept);		// intercept
  PARAMETER(theta); 		// logit of zero_prob
  PARAMETER(beta_lat);		// slope for longtitude
  PARAMETER(beta_lon);		// slope for latititude
  PARAMETER(logsigma);		// log of observation SD
  // Spatial parameters
  PARAMETER(logsigma_space);	// spatial variance
  PARAMETER(a);			// decay rate for covariance
  PARAMETER_VECTOR(u);		// spatial random effects

  // Objective funcction
  Type zero_prob = 1 / (1 + exp(-theta));
  Type sigma = exp(logsigma);
  Type sigma_space = exp(logsigma_space);
  int n = y.size();
  Type jnll=0;	 // used jnll

  // Linear predictor
  vector<Type> pred(n);
  pred = intercept + beta_lat*lat + beta_lon*lon +
    sigma_space*u;		// the spatial effect

  matrix<Type> cov(n,n);
  for (int i=0;i<n;i++){
    cov(i,i)=Type(1);
    for (int j=0;j<i;j++){
      // Exponentially decaying correlation
      cov(i,j)=exp(-a*dd(i,j));
      cov(j,i)=cov(i,j);
    }
  }

  // Probability of random effects. This replaces "dnorm" becuase it is a
  // multivaraite normal instead of iid.
  using namespace density;
  MVNORM_t<Type> neg_log_density(cov);
  jnll+=neg_log_density(u); // Note this returns the NLL!

  // Probability of data conditional on fixed effect values
  for( int i=0; i<n; i++){
    // If data is zero
    if(y(i)==0){
      jnll -= log( zero_prob );
    } else {
      // Positive data contribution
      jnll -= log( 1-zero_prob );
      // And the contribution of the likelihood
      if(likelihood==1)
      	jnll -= dinvgauss(y(i), exp(pred(i)), sigma, true);
      else if(likelihood==2)
      	jnll -= dlognorm(y(i), pred(i), sigma, true);
      else {
       	std::cout << "Invalid likelihood specified" << std::endl;
      	return 0;
      }
    }
  }

  // Reporting
  REPORT(zero_prob);
  REPORT(pred);
  REPORT(sigma);
  REPORT(sigma_space);
  REPORT(cov);
  REPORT(u);
  return jnll;
}
