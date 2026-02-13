#!/bin/bash

# RootStream Infrastructure Deployment Script
# This script automates the deployment of RootStream infrastructure

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Default values
ENVIRONMENT=${ENVIRONMENT:-production}
AWS_REGION=${AWS_REGION:-us-east-1}
CLUSTER_NAME="rootstream-cluster"

echo -e "${GREEN}=====================================${NC}"
echo -e "${GREEN}RootStream Infrastructure Deployment${NC}"
echo -e "${GREEN}=====================================${NC}"
echo ""

# Check dependencies
echo -e "${YELLOW}Checking dependencies...${NC}"

command -v terraform >/dev/null 2>&1 || { echo -e "${RED}terraform is required but not installed.${NC}" >&2; exit 1; }
command -v kubectl >/dev/null 2>&1 || { echo -e "${RED}kubectl is required but not installed.${NC}" >&2; exit 1; }
command -v helm >/dev/null 2>&1 || { echo -e "${RED}helm is required but not installed.${NC}" >&2; exit 1; }
command -v aws >/dev/null 2>&1 || { echo -e "${RED}aws CLI is required but not installed.${NC}" >&2; exit 1; }

echo -e "${GREEN}All dependencies found!${NC}"
echo ""

# Step 1: Deploy infrastructure with Terraform
echo -e "${YELLOW}Step 1: Deploying infrastructure with Terraform...${NC}"
cd ../terraform

if [ ! -d ".terraform" ]; then
    echo "Initializing Terraform..."
    terraform init
fi

echo "Planning Terraform deployment..."
terraform plan -out=tfplan

read -p "Apply Terraform plan? (yes/no): " apply_terraform
if [ "$apply_terraform" == "yes" ]; then
    echo "Applying Terraform configuration..."
    terraform apply tfplan
    
    # Get outputs
    EKS_CLUSTER_ENDPOINT=$(terraform output -raw eks_cluster_endpoint)
    RDS_ENDPOINT=$(terraform output -raw rds_endpoint)
    REDIS_ENDPOINT=$(terraform output -raw redis_endpoint)
    ECR_REPOSITORY=$(terraform output -raw ecr_repository_url)
    
    echo -e "${GREEN}Infrastructure deployed successfully!${NC}"
else
    echo "Skipping Terraform apply."
fi

cd ../scripts

# Step 2: Configure kubectl
echo ""
echo -e "${YELLOW}Step 2: Configuring kubectl...${NC}"
aws eks update-kubeconfig --region $AWS_REGION --name $CLUSTER_NAME

echo -e "${GREEN}kubectl configured!${NC}"

# Step 3: Build and push Docker image
echo ""
echo -e "${YELLOW}Step 3: Building and pushing Docker image...${NC}"

read -p "Build and push Docker image? (yes/no): " build_docker
if [ "$build_docker" == "yes" ]; then
    cd ../../
    
    # Login to ECR
    aws ecr get-login-password --region $AWS_REGION | docker login --username AWS --password-stdin $ECR_REPOSITORY
    
    # Build image
    docker build -t rootstream-server:latest -f infrastructure/docker/rootstream-server.Dockerfile .
    
    # Tag and push
    docker tag rootstream-server:latest $ECR_REPOSITORY:latest
    docker push $ECR_REPOSITORY:latest
    
    echo -e "${GREEN}Docker image built and pushed!${NC}"
    
    cd infrastructure/scripts
fi

# Step 4: Deploy with Helm
echo ""
echo -e "${YELLOW}Step 4: Deploying application with Helm...${NC}"

read -p "Deploy with Helm? (yes/no): " deploy_helm
if [ "$deploy_helm" == "yes" ]; then
    cd ../helm
    
    # Create namespace if it doesn't exist
    kubectl create namespace rootstream --dry-run=client -o yaml | kubectl apply -f -
    
    # Create secrets (you should replace these with actual values)
    kubectl create secret generic rootstream-db-secret \
        --from-literal=password=changeme \
        --namespace=rootstream \
        --dry-run=client -o yaml | kubectl apply -f -
    
    kubectl create secret generic rootstream-redis-secret \
        --from-literal=password=changeme \
        --namespace=rootstream \
        --dry-run=client -o yaml | kubectl apply -f -
    
    # Install or upgrade Helm chart
    helm upgrade --install rootstream ./rootstream \
        --namespace rootstream \
        --set image.repository=$ECR_REPOSITORY \
        --set image.tag=latest \
        --wait
    
    echo -e "${GREEN}Application deployed with Helm!${NC}"
    
    cd ../scripts
fi

# Step 5: Verify deployment
echo ""
echo -e "${YELLOW}Step 5: Verifying deployment...${NC}"

echo "Checking pods..."
kubectl get pods -n rootstream

echo ""
echo "Checking services..."
kubectl get services -n rootstream

echo ""
echo "Checking ingress..."
kubectl get ingress -n rootstream

# Print summary
echo ""
echo -e "${GREEN}=====================================${NC}"
echo -e "${GREEN}Deployment Summary${NC}"
echo -e "${GREEN}=====================================${NC}"
echo ""
echo -e "Environment: ${YELLOW}$ENVIRONMENT${NC}"
echo -e "AWS Region: ${YELLOW}$AWS_REGION${NC}"
echo -e "Cluster: ${YELLOW}$CLUSTER_NAME${NC}"

if [ ! -z "$RDS_ENDPOINT" ]; then
    echo -e "Database: ${YELLOW}$RDS_ENDPOINT${NC}"
fi

if [ ! -z "$REDIS_ENDPOINT" ]; then
    echo -e "Redis: ${YELLOW}$REDIS_ENDPOINT${NC}"
fi

echo ""
echo -e "${GREEN}Deployment completed!${NC}"
echo ""
echo "To check the status of your deployment:"
echo "  kubectl get all -n rootstream"
echo ""
echo "To view logs:"
echo "  kubectl logs -n rootstream -l app.kubernetes.io/name=rootstream"
echo ""
