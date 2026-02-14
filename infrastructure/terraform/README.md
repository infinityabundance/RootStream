# RootStream Infrastructure as Code (Terraform)

This directory contains Terraform configurations for provisioning RootStream infrastructure on AWS.

## Overview

The Terraform configuration provisions:
- VPC with public and private subnets across 3 availability zones
- EKS (Elastic Kubernetes Service) cluster
- RDS PostgreSQL database (Multi-AZ)
- ElastiCache Redis cluster
- Application Load Balancer
- S3 bucket for storage
- ECR repository for Docker images
- CloudWatch logging
- Security groups and IAM roles

## Architecture

```
┌─────────────────────────────────────────────────┐
│                  AWS Region                      │
│                                                  │
│  ┌───────────────────────────────────────────┐  │
│  │              VPC (10.0.0.0/16)            │  │
│  │                                           │  │
│  │  ┌─────────────┐    ┌─────────────┐     │  │
│  │  │   Public    │    │   Private   │     │  │
│  │  │   Subnets   │    │   Subnets   │     │  │
│  │  │             │    │             │     │  │
│  │  │  ┌───────┐  │    │  ┌───────┐  │     │  │
│  │  │  │  ALB  │  │    │  │  EKS  │  │     │  │
│  │  │  └───────┘  │    │  │ Nodes │  │     │  │
│  │  │             │    │  └───────┘  │     │  │
│  │  └─────────────┘    │             │     │  │
│  │                     │  ┌───────┐  │     │  │
│  │                     │  │  RDS  │  │     │  │
│  │                     │  └───────┘  │     │  │
│  │                     │             │     │  │
│  │                     │  ┌───────┐  │     │  │
│  │                     │  │ Redis │  │     │  │
│  │                     │  └───────┘  │     │  │
│  │                     └─────────────┘     │  │
│  └───────────────────────────────────────────┘  │
└─────────────────────────────────────────────────┘
```

## Files

- **main.tf**: Main infrastructure definitions
- **variables.tf**: Input variables
- **outputs.tf**: Output values
- **terraform.tfvars** (not included): Variable values (create this)

## Prerequisites

1. **Terraform** >= 1.0
   ```bash
   brew install terraform  # macOS
   # or download from terraform.io
   ```

2. **AWS CLI** configured
   ```bash
   aws configure
   ```

3. **AWS Credentials** with appropriate permissions

## Setup

### 1. Create terraform.tfvars

```hcl
aws_region = "us-east-1"
environment = "production"

# Database credentials (MUST use secure method in production)
# Use AWS Secrets Manager, environment variables, or vault
# NEVER commit actual passwords to version control
db_username = "rootstream_admin"
db_password = "USE_SECRETS_MANAGER_OR_ENV_VAR"

# Node configuration
node_desired_size = 3
node_min_size = 1
node_max_size = 10
```

### 2. Initialize Terraform

```bash
terraform init
```

This downloads required providers and sets up the backend.

### 3. Plan Deployment

```bash
terraform plan -out=tfplan
```

Review the planned changes before applying.

### 4. Apply Configuration

```bash
terraform apply tfplan
```

This provisions all infrastructure. Takes ~20-30 minutes.

## Variables

### Required Variables

| Variable | Description | Default |
|----------|-------------|---------|
| `db_username` | RDS master username | - |
| `db_password` | RDS master password | - |

### Optional Variables

| Variable | Description | Default |
|----------|-------------|---------|
| `aws_region` | AWS region | us-east-1 |
| `environment` | Environment name | production |
| `kubernetes_version` | K8s version | 1.27 |
| `node_instance_type` | EC2 instance type | t3.xlarge |
| `node_desired_size` | Desired node count | 3 |
| `node_min_size` | Minimum nodes | 1 |
| `node_max_size` | Maximum nodes | 10 |
| `db_instance_class` | RDS instance class | db.t3.large |
| `db_allocated_storage` | RDS storage (GB) | 100 |
| `db_multi_az` | Multi-AZ deployment | true |
| `redis_node_type` | Redis node type | cache.t3.medium |
| `redis_num_nodes` | Redis node count | 3 |
| `log_retention_days` | Log retention | 30 |

## Outputs

After deployment, Terraform outputs important information:

```bash
terraform output
```

### Key Outputs

- `eks_cluster_endpoint`: EKS cluster API endpoint
- `rds_endpoint`: Database endpoint
- `redis_endpoint`: Redis cache endpoint
- `alb_dns_name`: Load balancer DNS
- `s3_bucket_name`: Storage bucket name
- `ecr_repository_url`: Docker registry URL

### Get Specific Output

```bash
terraform output eks_cluster_endpoint
terraform output -json  # All outputs as JSON
```

## State Management

### Remote State (Recommended)

Configure S3 backend in `main.tf`:

```hcl
terraform {
  backend "s3" {
    bucket = "rootstream-terraform-state"
    key    = "infrastructure/terraform.tfstate"
    region = "us-east-1"
  }
}
```

### State Commands

```bash
# List resources in state
terraform state list

# Show resource details
terraform state show aws_eks_cluster.rootstream

# Import existing resource
terraform import aws_eks_cluster.rootstream rootstream-cluster
```

## Resource Management

### Update Infrastructure

1. Modify Terraform files
2. Plan changes: `terraform plan`
3. Apply changes: `terraform apply`

### Destroy Infrastructure

**Warning**: This deletes all resources!

```bash
terraform destroy
```

### Target Specific Resources

```bash
# Plan specific resource
terraform plan -target=aws_eks_cluster.rootstream

# Apply specific resource
terraform apply -target=aws_eks_cluster.rootstream
```

## Cost Estimation

### Monthly Cost Breakdown (Approximate)

| Resource | Instance Type | Estimated Cost |
|----------|---------------|----------------|
| EKS Cluster | - | $73/month |
| EC2 Nodes (3x) | t3.xlarge | $300/month |
| RDS Database | db.t3.large | $200/month |
| Redis Cluster (3x) | cache.t3.medium | $150/month |
| Load Balancer | - | $20/month |
| S3 Storage | - | Variable |
| Data Transfer | - | Variable |
| **Total** | | **~$750/month** |

*Costs vary by region and usage*

### Cost Optimization

1. Use Reserved Instances for predictable workloads
2. Enable auto-scaling to match demand
3. Use spot instances for non-critical workloads
4. Right-size instances based on monitoring
5. Implement S3 lifecycle policies

## Security

### Best Practices

1. **Never commit terraform.tfvars** with secrets
2. **Use AWS Secrets Manager** for sensitive data
3. **Enable MFA** for AWS account
4. **Restrict S3 bucket** access
5. **Enable CloudTrail** logging
6. **Use least privilege** IAM policies
7. **Enable encryption** everywhere

### Secrets Management

```hcl
# Use AWS Secrets Manager
data "aws_secretsmanager_secret_version" "db_password" {
  secret_id = "rootstream/db/password"
}

resource "aws_db_instance" "rootstream" {
  password = data.aws_secretsmanager_secret_version.db_password.secret_string
}
```

## Monitoring

### CloudWatch

- Logs: `/aws/eks/rootstream`
- Metrics: EKS, RDS, ElastiCache
- Alarms: Set up for critical metrics

### Terraform Drift Detection

```bash
terraform plan -refresh-only
```

## Backup and Disaster Recovery

### RDS Backups

- Automated backups: 7 days retention
- Manual snapshots: On-demand
- Point-in-time recovery enabled

### Disaster Recovery

```bash
# Export Terraform state
terraform state pull > terraform.tfstate.backup

# Restore from backup
terraform state push terraform.tfstate.backup
```

## Troubleshooting

### Common Issues

**1. State Lock Error**

```bash
# Force unlock (use carefully)
terraform force-unlock <lock-id>
```

**2. Resource Already Exists**

```bash
# Import existing resource
terraform import <resource_type>.<name> <resource_id>
```

**3. Timeout Errors**

Increase timeout in resource configuration.

**4. Permission Denied**

Check IAM permissions for your AWS credentials.

## Advanced Usage

### Workspaces

Manage multiple environments:

```bash
terraform workspace new staging
terraform workspace select production
terraform workspace list
```

### Modules

Break down into reusable modules:

```hcl
module "eks" {
  source = "./modules/eks"
  cluster_name = var.cluster_name
}
```

## CI/CD Integration

### GitHub Actions Example

```yaml
- name: Terraform Init
  run: terraform init

- name: Terraform Plan
  run: terraform plan -out=tfplan

- name: Terraform Apply
  if: github.ref == 'refs/heads/main'
  run: terraform apply -auto-approve tfplan
```

## Compliance

- HIPAA compliant configurations available
- PCI-DSS considerations
- SOC 2 aligned practices

## Support

For issues:
1. Check AWS service health
2. Review CloudWatch logs
3. Consult Terraform documentation
4. Check AWS support resources

## License

MIT License - See root LICENSE file
